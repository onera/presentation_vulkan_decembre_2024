#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cassert>
#include <chrono>
#include <cmath>
#include <thread>
#include <string>
using namespace std::string_literals;
#include "ansi.hpp"
#include "debug_utils.hpp"
#include "vk_computing.hpp"

namespace vulkan {
constexpr const int workgroup_size = 32; // Taille des groupes de travail dans le shader de calcul
// ====================================================================================================================
void
ComputingPipeline::create_instance()
{
    /*
    En activant une couche de validation, Vulkan émettera des alertes si l'API est utilisée incorrectement.
    Vulkan est une API explicite, permettant un contrôle direct de la façon dont un GPU fonctionne vraiment.
    Vulkan est conçu afin qu'une vérification minimale 
    */
    if constexpr (enableValidationLayers)
    {
        auto layer_properties = vk::enumerateInstanceLayerProperties();
        bool has_validation_layer_type = false;
        for (auto prop : layer_properties)
        {
            if (strcmp(debug::Utils::needed_layer_name, prop.layerName) == 0) 
                has_validation_layer_type = true;
        }// for (auto prop : ....)
        if (not has_validation_layer_type)
            std::cerr << "Layer de validation non pris en compte !!!" << std::endl;

        auto extensions = vk::enumerateInstanceExtensionProperties();
        for (auto prop : extensions)
        {
            if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, prop.extensionName) == 0)
            {
                this->enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }
        if (this->enabled_extensions.empty())
        {
            std::cerr << "Attention : extension vulkan de deboggage non disponible sur cette plateforme" << std::endl;
        }
    }// if constexpr (enableValidationLayers)
    /** Création d'une instance : en Vulkan, il n'existe pas de contexte global et tous les états gérés par une application sont stockés dans une
     *  instance Vulkan. En créant une instance Vulkan, on initialise la bibliothèque Vulkan et permet à l'application de passer des informations
     *  sur elle-même pour la mise en oeuvre.
    */
    vk::ApplicationInfo application_info;
    application_info.setApiVersion(vk::ApiVersion13)
                    .setPApplicationName("Computing shader")
                    .setApplicationVersion(0)
                    .setPEngineName("Computing engine")
                    .setEngineVersion(0);

    vk::InstanceCreateInfo create_info;
    create_info.setPApplicationInfo(&application_info)
               .setEnabledLayerCount(uint32_t(this->enabled_layers.size()))
               .setPpEnabledLayerNames(enabled_layers.data())
               .setEnabledExtensionCount(uint32_t(this->enabled_extensions.size()))
               .setPpEnabledExtensionNames(this->enabled_extensions.data());
    
    this->instance = vk::createInstance(create_info);
}
// --------------------------------------------------------------------------------------------------------------------
void 
ComputingPipeline::find_physical_device()
{
    // Dans un premier temps, on extrait la liste des périphériques physiques disponibles sur la plateforme :
    auto devices = this->instance.enumeratePhysicalDevices();
    if (devices.empty()) throw std::runtime_error("Impossible de trouver un périphérique compatible avec Vulkan !");
    /*
    Ensuite, on choisit un périphérique qui peut être utilisé pour notre problème.

    Avec la méthode getFeatures() de vk::Device, on peut obtenir une liste fine des propriétés physiques des périphériques
    supportées par la plateforme. Dans cet exemple, on essaiera d'avoir en priorité une carte graphique avec mémoire dédiée
    puis si non valable une carte graphique utilisant la RAM puis en dernier lieu le processeur lui-même.

    Avec la méthode getProperties() de vk::Device, on peut obtenir une liste des propriétés globales du périphérique physique.
    Le plus important étant qu'on peut obtenir une liste des limitations du périphérique. Pour notre application, on veut pouvoir
    exécuter un shader de calcul or la taille maximale du groupe d'exécution, et le nombre total d'invocations d'un shader de calcul
    est limité par le périphérique. On doit donc s'assurer que les limitations nommées maxComputeWorkGroupCount, maxComputeWorkGroupInvocations
    et maxComputeWorkGroupSize ne sont pas dépassées par ce que demande notre application. De plus, on utilise un buffer de stockage dans le
    shader de calcul, et on veut s'assurer que la taille demandée n'est pas plus grande que ce que notre périphérique peut gérer en vérifiant
    la valeur limite donnée par maxStorageBufferRange.

    En fait, dans notre application, la taille du groupe de calcul et le nombre total d'invocation du shader de calcul sont relativement faibles,
    et le buffer de stockage n'est pas très grand si bien qu'une grande majorité des périphériques peut gérer les ressources demandées. On peut vérifier
    cela en regardant certains périphériques sur la page internet http://vulkan.gpuinfo.org/

    Ici, afin de garder les choses simples et propres, on se contente uniquement de s'assurer d'utiliser un GPU avec si possible de la mémoire dédiée. Mais
    dans une application réelle et sérieuse, ces limitations doivent être prises en compte.

    Pour cela, on va calculer un "poids" pour chaque périphérique est conservé uniquement celui qui a le plus grand poids (car susceptible d'être le périphérique le plus
    efficace pour notre calcul).
    */
    int32_t index_device = -1;
    int max_priority     = -1;
    int ind = 0;
    for (auto dev : devices)
    {
        int priority = 0;
        auto features    = dev.getFeatures();
        auto features2   = dev.getFeatures();
        auto dev_memory = dev.getMemoryProperties();
        auto properties = dev.getProperties();
        vk::PhysicalDeviceType type =  properties.deviceType;
        if (type == vk::PhysicalDeviceType::eDiscreteGpu) priority += 1024;
        else if (type == vk::PhysicalDeviceType::eIntegratedGpu) priority += 512;
        
        // On recherche si le périphérique possède sa propre mémoire :
        auto mem_types = dev_memory.memoryTypes;
        for (auto mem : mem_types)
        {
            if (mem.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) priority += 16;
            if (mem.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) priority += 2 ;
        }
        if (priority > max_priority)
        {
            max_priority = priority;
            index_device = ind;
        }
        ++ind;
    }
    this->physical_device = devices[index_device];
}
// --------------------------------------------------------------------------------------------------------------------
uint32_t 
ComputingPipeline::get_compute_queue_family_index()
{
    // Avoir toutes les familles de queues proposées par le périphérique
    auto queue_families = this->physical_device.getQueueFamilyProperties();

    // Trouver dans ces familles une famille supportant les shaders de calcul :
    uint32_t i = 0;
    for (; i < queue_families.size(); ++i) 
    {
        auto props = queue_families[i];
        if ( (props.queueCount > 0) && (props.queueFlags & vk::QueueFlagBits::eCompute) )
        {
            // OK, on a bien trouvé une famille de queues avec calcul. On la garde !
            break;
        }
    }

    if (i == queue_families.size()) 
    {
        throw std::runtime_error("Le périphérique ne propose pas de famille de queues étant capable de faire des calculs");
    }

    return i;
}
// --------------------------------------------------------------------------------------------------------------------
void 
ComputingPipeline::create_device()
{
    this->queue_family_index = get_compute_queue_family_index();
    // On crée un périphérique logique dans cette fonction
    // Lorsqu'on crée le périphérique logique, on spécifie la famille de queues qu'il devra utiliser :
    std::vector queue_priority{1.f}; 
    vk::DeviceQueueCreateInfo queue_create_info;
    queue_create_info.setQueueFamilyIndex(this->queue_family_index)
                     .setQueueCount(1)
                     .setQueuePriorities(queue_priority);// Quand on a plusieurs queues, on peut leur attribuer des priorités. Ici une queue, donc aucune importance
    
    // On va maintenant créer notre périphérique logique qui nous permettra d'interagir avec le périphérique physique.
    // On peut spécifier toutes les caractéristiques désirées pour notre périphérique. On n'en a besoin d'aucune pour cette application...
    // On prend donc l'objet construit par défaut.
    vk::PhysicalDeviceFeatures device_features;
    vk::DeviceCreateInfo device_create_info;
    device_create_info.setEnabledLayerCount(this->enabled_layers.size())
                      .setPpEnabledLayerNames(this->enabled_layers.data())
                      .setPQueueCreateInfos(&queue_create_info)
                      .setQueueCreateInfoCount(1)
                      .setPEnabledFeatures(&device_features);

    this->logical_device = this->physical_device.createDevice(device_create_info, nullptr);

    // On récupère un lien sur le seul membre de la famille de queues (d'indice 0 donc)
    this->queue = this->logical_device.getQueue(queue_family_index, 0);
}
// --------------------------------------------------------------------------------------------------------------------
uint32_t 
ComputingPipeline::find_memory_type(uint32_t memory_type_bits, vk::MemoryPropertyFlags properties)
{
    auto memory_properties = this->physical_device.getMemoryProperties();
    // Voir la documentation Vulkan de VkPhysicalDeviceMemoryProperties pour voir les différentes propriétés possibles
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ( (memory_type_bits & (1 << i)) && ( (memory_properties.memoryTypes[i].propertyFlags & properties) == properties))
            return i;
    }
    return -1;
}
// --------------------------------------------------------------------------------------------------------------------
void
ComputingPipeline::create_buffer()
{
    // On va créer un buffer. On calculera l'ensemble de mandelbrot dans ce buffer à l'aide d'un shader de calcul
    vk::BufferCreateInfo buffer_create_info;
    buffer_create_info.setSize(buffer_size)                              // La taille du buffer en octets
                      .setUsage(vk::BufferUsageFlagBits::eStorageBuffer) // Le buffer sera utilisé comme buffer de stockage
                      .setSharingMode(vk::SharingMode::eExclusive);      // Et il sera exclusif : une seule famille de queue peut le manipuler en même temps
    this->buffer = this->logical_device.createBuffer(buffer_create_info, nullptr);

    //! ATTENTION : le buffer n'alloue pas de lui-même la mémoire. On doit le faire manuellement
    //! ========================================================================================
    // On doit dans une premier temps trouver les besoins mémoires du buffer :
    auto memory_requirements = this->logical_device.getBufferMemoryRequirements(buffer);    
    // Que nous allons utilisé pour allouer la mémoire pour le buffer :
    vk::MemoryAllocateInfo allocate_info;
    allocate_info.setAllocationSize(memory_requirements.size);
    /*
    Il existe plusieurs types de mémoire qui peuvent être alloués et on doit choisir un type de mémoire qui :
      1) qui vérifie les besoins mémoires (memory_requirements.memoryTypeBits)
      2) qui vérifie nos propres besoins. On veut être capable de lire un buffer mémoire du GPU vers le CPU
         en mappant la mémoire GPU sur le CPU. Alors on a comme besoin le bit vk::MemoryPropertyFlagBits::eHostVisible
         De plus; en imposant le flag vk::MemoryPropertyFlagBits::eHostCached, la mémoire écrite par le périphérique (GPU)
         sera facilement visible par l'hôte (CPU) sans avoir à appeler des commandes supplémentaires de flush. On met donc principalement
         ce flag pour simplifier le code.
    */
    //allocate_info.setMemoryTypeIndex(find_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostCoherent |
    allocate_info.setMemoryTypeIndex(find_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostCached     /*|
                                                                                          vk::MemoryPropertyFlagBits::eHostVisible*/) );
    this->buffer_memory = this->logical_device.allocateMemory(allocate_info, nullptr);
    // On associe maintenant la mémoire allouée au buffer :
    this->logical_device.bindBufferMemory(this->buffer, this->buffer_memory, 0); // Dernier paramètre : offset
}
// --------------------------------------------------------------------------------------------------------------------
void
ComputingPipeline::create_descriptor_set_layout()
{
    // On spécifie ici la disposition d'un ensemble de descripteurs servant à relier nos objets vulkans aux ressources dans les shaders
    // On spécifie ici un lien de type vk::DescriptorType::eStorageBuffer au point de liaison 0. Ceci correspond au lien
    //
    //    layout(std140, binding = 0) buffer buff
    //
    // dans le shader de calcul (seul objet vulkan partagé par les threads qu'on utilisera).
    vk::DescriptorSetLayoutBinding descriptor_set_layout_binding;
    descriptor_set_layout_binding.setBinding(0)
                                 .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                                 .setDescriptorCount(1)
                                 .setStageFlags(vk::ShaderStageFlagBits::eCompute);
    // Ainsi, notre disposition (layout) ne contiendra qu'un seul descripteur :
    vk::DescriptorSetLayoutCreateInfo create_info;
    create_info.setBindingCount(1)
               .setPBindings(&descriptor_set_layout_binding);
    // Création de la disposition de l'ensemble de descripteur :
    this->descriptor_set_layout = this->logical_device.createDescriptorSetLayout(create_info, nullptr);
}
// --------------------------------------------------------------------------------------------------------------------
void 
ComputingPipeline::create_descriptor_set()
{
    /*
    Nous allons allouer un ensemble de descripteur ici. Pour cela, on doit d'abord créer une ressource de descripteurs.
    Puisqu'on n'a besoin que d'un seul descripteur, notre réservoir ne pourra en allouer qu'un seul...
    */
    vk::DescriptorPoolSize descriptor_pool_size;
    descriptor_pool_size.setType(vk::DescriptorType::eStorageBuffer);
    descriptor_pool_size.setDescriptorCount(1);

    vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
    descriptor_pool_create_info.setMaxSets(1)
                               .setPoolSizeCount(1)
                               .setPPoolSizes(&descriptor_pool_size);
    
    // Créer la ressource pour allouer des (ici un  seul !) descripteur(s).
    this->descriptor_pool = this->logical_device.createDescriptorPool(descriptor_pool_create_info, nullptr);

    // Avec les ressources créées, on peut maintenant allouer l'ensemble des descripteurs
    vk::DescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.setDescriptorPool(descriptor_pool)
                                .setDescriptorSetCount(1)
                                .setPSetLayouts(&this->descriptor_set_layout);
    // Allocation de(s) ensemble(s) de descripteurs. La méthode renvoie un tableau dynamique...
    auto descriptors_set = this->logical_device.allocateDescriptorSets(descriptor_set_allocate_info);
    this->descriptor_set = descriptors_set[0];

    // Pour GLSL, on a besoin de connecter notre buffer de stockage actuel avec un descripteur.
    // On va utiliser la méthode updateDescriptorSets de logic_device pour mettre à jour notre ensemble de descripteurs
    vk::DescriptorBufferInfo descriptor_buffer_info;
    descriptor_buffer_info.setBuffer(this->buffer)
                          .setOffset(0)
                          .setRange(this->buffer_size);
    vk::WriteDescriptorSet write_descriptor_set;
    write_descriptor_set.setDstSet(this->descriptor_set) // Ecrire dans cet ensemble de descripteur
                        .setDstBinding(0)                // écrire sur le premier descripteur et seulement faire une liaison
                        .setDescriptorCount(1)           // On n'utilise qu'un seul descripteur
                        .setDescriptorType(vk::DescriptorType::eStorageBuffer) // et c'est un buffer de stockage
                        .setPBufferInfo(&descriptor_buffer_info);
    // On effectue la mise à jour de l'ensemble des descripteurs :
    this->logical_device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);
}
// --------------------------------------------------------------------------------------------------------------------
namespace __details__
{
std::vector<uint32_t> read_file(char const *filename)
{
    // On lit le code compilé binaire du shader de calcul qui a été compilé à l'aide de glslangValidator :
    //     glslangValidator -V shader.comp
    // ou bien (autre compilateur)
    //     glslc -O -o comp.spv shader.comp
    // etc.

    std::ifstream fichier(filename, std::ios_base::binary);
    std::size_t length = 0;
    if (fichier)
    {
        fichier.seekg(0, std::ios_base::end);// On va à la fin du fichier
        length = fichier.tellg();
        fichier.seekg(0, std::ios_base::beg);// On revient au début du fichier
    }
    else 
    {
        std::string error_msg = "Impossible de trouver ou d'ouvrir le fichier "s + std::string(filename);
        throw std::runtime_error(error_msg.c_str());
    }
    // Dans les spécifications de vulkan, il est indiqué que le binaire de chaque shaders qu'on va utiliser dans Vulkan
    // doit être stocké dans un tableau dont la taille en octet doit être un multiple de quatre.
    // L'idée est donc de remplir les derniers octets non utilisés par une valeur nulle.
    std::size_t length_padded = std::size_t(std::ceil(length/4.));
    // D'où l'utilisation du type uint32_t pour le stockage, car cela nous garantit cette multiplicité par quatre (car uint32_t prend quatre octets en mémoire)
    std::vector<uint32_t> binary(length_padded);
    fichier.read((char *)binary.data(), length);
    if (not fichier)
    {
        std::string error_msg = "Erreur de lecture en lisant le fichier "s + std::string(filename);
        throw std::runtime_error(error_msg.c_str());
    }
    char* binary_data = (char *)binary.data();
    for (std::size_t i = length; i < 4*length_padded; ++i)
        binary_data[i] = char(0);
    return binary;
}
}
void 
ComputingPipeline::create_compute_pipeline()
{
    // On crée le pipeline de calcul ici
    // Crée un module de shader. Un shader module ne fait qu'encapsuler le code shader
    // Le code dans comp.spv a été crée en exécutant la commande suivante :
    //    glslangValidator -V shader.comp
    auto code = __details__::read_file("shaders/comp.spv");

    vk::ShaderModuleCreateInfo create_info;
    create_info.setPCode(code.data())
               .setCodeSize(sizeof(uint32_t)*code.size());

    this->compute_shader_module = this->logical_device.createShaderModule(create_info, nullptr);

    /*
    Nous allons créer enfin le pipeline de calcul.
    Un pipeline de calcul est très simple par rapport à un pipeline graphique.
    Un pipeline de calcul n'est constitué que d'une seule étape.

    On spécifie donc en premier lieu l'étape de calcul par shader de calcul et son point d'entrée (main)
    */
    vk::PipelineShaderStageCreateInfo shader_stage_create_info;
    shader_stage_create_info.setStage(vk::ShaderStageFlagBits::eCompute)
                            .setModule(this->compute_shader_module)
                            .setPName("main");
    /* 
     * La disposition suivant permet au pipeline d'accèder aux ensembles des descripteurs.
     * Nous n'avons qu'à spécifier la disposition de l'ensemble des descripteurs que nous avons crée plus tôt
     * Note : L'ordre des layout pour les descriptions donnera le n° du set dans le shader (si set par précisé dans shader, vaut 0) !
    */
    vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
    pipeline_layout_create_info.setSetLayoutCount(1)
                               .setPSetLayouts(&this->descriptor_set_layout);
    this->pipeline_layout = this->logical_device.createPipelineLayout(pipeline_layout_create_info, nullptr);

    std::vector<vk::ComputePipelineCreateInfo> pipeline_create_infos(1);
    pipeline_create_infos[0].setStage(shader_stage_create_info);
    pipeline_create_infos[0].setLayout(this->pipeline_layout);

    auto pipelines = this->logical_device.createComputePipelines(vk::PipelineCache{nullptr}, pipeline_create_infos);
    this->pipeline = pipelines.value[0];
}
// --------------------------------------------------------------------------------------------------------------------
void 
ComputingPipeline::create_command_buffer()
{
    /*
    On arrive à la fin. Afin d'envoyer des commandes au périphérique (GPU),
    on doit d'abord enregistrer les commandes dans un buffer.
    Pour allouer le buffer de commande, on doit d'abord créer une ressource de commande.
    C'est ce qu'on fait en premier.
    */
    vk::CommandPoolCreateInfo command_pool_create_info;
    command_pool_create_info.setQueueFamilyIndex(this->queue_family_index);
    this->command_pool = this->logical_device.createCommandPool(command_pool_create_info, nullptr);

    // On alloue maintenant un buffer de commande de la ressource de commandes
    vk::CommandBufferAllocateInfo command_buffer_allocate_info;
    command_buffer_allocate_info.setCommandPool(this->command_pool);// On spécifie la ressource à partir de laquelle on alloue notre buffer
    // Si le buffer de commande est primaire, il peut être directement soumis aux queues
    // Un buffer secondaire a à être appelé d'un' buffer primaire de commande, et ne peut pas être directement
    // soumis à une queue. Par soucis de simplicité, on utilise ici un buffer de commande primaire
    command_buffer_allocate_info.setLevel(vk::CommandBufferLevel::ePrimary)
                                .setCommandBufferCount(1);// On alloue qu'une seul buffer de commande
    auto command_buffers = this->logical_device.allocateCommandBuffers(command_buffer_allocate_info);// Allocation du buffer de commande
    this->command_buffer = command_buffers[0];

    //this->command_buffer.writeTimestamp(...)

    // On peut commencer à enregistrer les commandes dans le buffer qui vient d'être alloué
    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // Le buffer est seulement soumis et utilisé une seule fois dans cette application
    this->command_buffer.begin(begin_info);// On commence l'enregistrement
    /*
    On a besoin de relier un pipeline ET un ensemble de descripteurs avant d'envoyer les commandes

    La couche de validation NE DONNE PAS d'alertes si on oublie cette étape, donc bien faire attention de ne pas oublier cette étape
    */
    this->command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute,this->pipeline);
    this->command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, this->pipeline_layout, 0, 1, &this->descriptor_set, 0, nullptr);

    /*
    Appelé la méthode dispatch du buffer de commande qui commence à calculer le pipeline et exécuter le shader de calcul.
    Le nombre de groupes de travail est spécifié dans les argumetns
    Si vous êtes familiés avec les shaders de calcul en OpenGL, rien de nouveau sous le soleil ici.   
    */
    this->command_buffer.dispatch( uint32_t(std::ceil(width/float(workgroup_size))), uint32_t(std::ceil(height/float(workgroup_size))), 1 );

    this->command_buffer.end();// Fin de l'enregistrement des commandes
}
// --------------------------------------------------------------------------------------------------------------------
void 
ComputingPipeline::run_command_buffer()
{
    // On va soumettre le buffer de command enregistré à une queue
    vk::SubmitInfo submit_info;
    submit_info.setCommandBufferCount(1)                  // On ne soumet qu'un seul buffer de commande
               .setPCommandBuffers(&this->command_buffer);// Le buffer de command à soumettre

    // On crée une barrière
    vk::Fence fence;
    vk::FenceCreateInfo fence_create_info;// Les valeurs par défaut sont ok ici
    fence = this->logical_device.createFence(fence_create_info, nullptr);

    // On soumet le buffer de command sur la queue, en donnant en même temps la barrière de synchronisation
    vk::Result result = this->queue.submit(1, &submit_info, fence );
    if (result != vk::Result::eSuccess)
    {
        std::cerr << ansi::BRed << ansi::Black << " Erreur " << ansi::Normal << " lors de la soumission du buffer de commande à la queue" << std::endl;
    }
    /*
    La commande ne se terminera pas jusqu'à ce que la barrière émet un signal. On attend donc ce signal.
    On pourra lire le buffer directement du GPU  après cette synchronisation en attendant la barrière.
    */
    result = this->logical_device.waitForFences(1, &fence, vk::True, 100'000'000'000);// Dernier chiffre = time out
    if (result != vk::Result::eSuccess)
    {
        std::cerr << ansi::BRed << ansi::Black << " Erreur " << ansi::Normal << " lors de la synchronisation avec la barrière" << std::endl;
    }

    this->logical_device.destroyFence(fence, nullptr);
}
// --------------------------------------------------------------------------------------------------------------------
void
ComputingPipeline::clean_up()
{
    if constexpr (enableValidationLayers) 
        this->pt_dbg_utils = nullptr;

    this->logical_device.freeMemory(this->buffer_memory, nullptr);
    this->logical_device.destroyBuffer(this->buffer, nullptr);
    this->logical_device.destroyShaderModule(this->compute_shader_module, nullptr);
    this->logical_device.destroyDescriptorPool(this-> descriptor_pool, nullptr);
    this->logical_device.destroyDescriptorSetLayout( this->descriptor_set_layout, nullptr);
    this->logical_device.destroyPipeline(this->pipeline, nullptr);
    this->logical_device.destroyCommandPool( this->command_pool, nullptr);
    this->logical_device.destroy();
    this->instance.destroy();
}
// ====================================================================================================================
void ComputingPipeline::cpu_computation()
{
    std::vector<Pixel> mandelbrot(width * height);
    std::cout << "mandelbrot address : " << mandelbrot.data() << std::endl;
    auto beg_time = std::chrono::high_resolution_clock::now();
#   pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < height; ++i)
    {
        float y = float(i)/float(height);
        for (int j = 0; j < width; ++j)
        {
            float n  = 0.f;
            float x  = float(j)/float(width);
            float cx = -0.445f + (x-0.5f)*(2.f + 1.7f*0.2f);
            float cy =  0.0f   + (y-0.5f)*(2.f + 1.7f*0.2f);
            float zr =  0.0f;
            float zi =  0.0f;

            constexpr const int M = 512;
            for (int iter=0; iter < M; ++iter)
            {
                float temp = zr*zr - zi*zi + cx;
                zi = 2*zr*zi + cy;
                zr = temp;
                if (zi*zi + zr*zr > 2) break;
                n += 1;
            }
            float t = float(n)/float(M);
            float d[] = {0.3, 0.3, 0.5};
            float e[] = {-0.2, -0.3 ,-0.5};
            float f[] = {2.1, 2.0, 3.0};
            float g[] = {0.0, 0.1, 0.0};
            mandelbrot[i*width+j].r = d[0] + e[0]*std::cos(6.28318f*(f[0]*t+g[0]));
            mandelbrot[i*width+j].g = d[1] + e[1]*std::cos(6.28318f*(f[1]*t+g[1]));
            mandelbrot[i*width+j].b = d[2] + e[2]*std::cos(6.28318f*(f[2]*t+g[2]));
            mandelbrot[i*width+j].a = 1.f;
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::cout << "Temps calcul mandelbrot sur cpu = " << std::chrono::duration<double, std::milli>(end_time - beg_time).count() << "[ms]" << std::endl;

    auto beg_time2 = std::chrono::high_resolution_clock::now();
    save_rendered_image(mandelbrot.data(), "mandelbrot_cpu.png");
    auto end_time2 = std::chrono::high_resolution_clock::now();
    std::cout << "Temps enregistrement mandelbrot cpu = " << std::chrono::duration<double, std::milli>(end_time2 - beg_time2).count() << "[ms]" << std::endl;
}
// --------------------------------------------------------------------------------------------------------------------
void
ComputingPipeline::run()
{
    using namespace std::chrono_literals;
    // Calcul de la taille du buffer à réserver pour le GPU afin de rendre l'ensemble de mandelbrot
    this->buffer_size = width * height * sizeof(Pixel);

    // Initialisation et configuration de Vulkan :
    create_instance();
    this->pt_dbg_utils = std::make_unique<vulkan::debug::Utils>(this->instance);
    find_physical_device();
    create_device();
    create_buffer();
    create_descriptor_set_layout();
    create_descriptor_set();
    create_compute_pipeline();
    create_command_buffer();

    // Enfin, on exécute le buffer de command enregistré :
    auto beg_time = std::chrono::high_resolution_clock::now();
    this->pt_dbg_utils->create_messenger();
    run_command_buffer();
    this->pt_dbg_utils->destroy_messenger();
    auto end_time = std::chrono::high_resolution_clock::now();
    std::cout << "Temps calcul mandelbrot sur gpu = " << std::chrono::duration<double, std::milli>(end_time - beg_time).count() << "[ms]" << std::endl;

    // On sauvegarde l'image calculé dans le buffer en png :
    // mappe la mémoire buffer alors qu'on puisse le lire du CPU :
    auto beg_time3 = std::chrono::high_resolution_clock::now();
    void *mapped_memory = this->logical_device.mapMemory(this->buffer_memory, 0, this->buffer_size);
    std::cout << "adresse memoire : " << mapped_memory << std::endl;
    auto end_time3 = std::chrono::high_resolution_clock::now();
    std::cout << "Temps mappage mandelbrot gpu vers CPU : " << std::chrono::duration<double, std::milli>(end_time3 - beg_time3).count() << "[ms]" << std::endl;
    //std::this_thread::sleep_for(5s);
    auto beg_time4 = std::chrono::high_resolution_clock::now();
    Pixel* pmapped_memory = (Pixel *)mapped_memory;
    save_rendered_image(pmapped_memory, "mandelbrot_gpu.png");
    auto end_time4 = std::chrono::high_resolution_clock::now();
    std::cout << "Temps sauvegarde mandelbrot gpu : " << std::chrono::duration<double, std::milli>(end_time4 - beg_time4).count() << "[ms]" << std::endl;
    // Une fois fait, on détruit la map entre CPU et buffer :
    auto beg_time2 = std::chrono::high_resolution_clock::now();
    this->logical_device.unmapMemory(this->buffer_memory);
    auto end_time2 = std::chrono::high_resolution_clock::now();
    std::cout << "Temps unmappage mandelbrot gpu vs CPU : " << std::chrono::duration<double, std::milli>(end_time2 - beg_time2).count() << "[ms]" << std::endl;

    // On libère toutes les ressources prises par Vulkan :
    clean_up();
}

}