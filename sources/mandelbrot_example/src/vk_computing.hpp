#include <memory>
#include <vulkan/vulkan.hpp>
#include "debug_utils.hpp"
#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

namespace vulkan
{
constexpr const int width  = 3200; // Taille en x du rendu de l'ensemble de mandelbrot
constexpr const int height = 2400; // Taille en y du rendu de l'ensemble de mandelbrot

class ComputingPipeline
{
public:
    struct Pixel { float r,g, b, a; };

    ComputingPipeline() = default;

    /**
     * @brief Créer une instance du contexte vulkan.
     * 
     */
    void create_instance();
    /**
     * @brief Sélectionne un périphérique compatible avec Vulkan sur la plateforme exécutant le code.
     * 
     * Lorsque plusieurs périphériques compatibles Vulkan sont disponibles, on va choisir en priorité un GPU dédié (et non
     * intégré avec un CPU) et si possible possédant sa propre mémoire vidéo (et non utilisant de la RAM).
     */
    void find_physical_device();
    /**
     * @brief Trouve et renvoie l'index d'une famille de queues pouvant effectuer des "compute shader"
     * 
     * @return uint32_t L'index de la famille de queue
     */
    uint32_t get_compute_queue_family_index();

    /**
     * @brief Fonction qui va créer à partir du périphérique physique choisi une vue logique du périphérique qui permettra de manipuler le périphérique physique
     *        à l'aide d'une couche abstraite logicielle.
     * 
     */
    void create_device();

    /**
     * @brief Recherche parmi la collection des types mémoires proposée par le périphérique une mémoire qui vérifie les propriétés demandées dans properties
     * 
     * @param memory_type_bits Les types de mémoires allouables pour le buffer dont on veut réserver la mémoire (voir https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements.html)
     * @param properties  Les propriétés demandées à la mémoire (voir https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryPropertyFlagBits.html pour les différentes propriétés possibles)
     * @return uint32_t L'indice de la mémoire correspondant à la mémoire vérifiant les propriétés demandées
     */
    uint32_t find_memory_type(uint32_t memory_type_bits, vk::MemoryPropertyFlags properties);

    /**
     * @brief Crée un buffer de stockage qui servira pour stocker l'image de mandelbrot.
     * 
     */
    void create_buffer();

    //@name Gestion des descripteurs
    //@{
    /**
     * @brief Décrit les collections de descripteurs 
     * 
     */
    void create_descriptor_set_layout();

    /**
     * @brief Créer un ensemble de descripteurs
     * 
     */
    void create_descriptor_set();
    //@}

    /**
     * @brief Créer le pipeline de calcul
     * 
     */
    void create_compute_pipeline();

    void create_command_buffer();

    void run_command_buffer();

    void run();

    void save_rendered_image(Pixel* pmapped_memory, std::string const& filename);

    void clean_up();

    void cpu_computation();
private:    
    /**
     * @brief Une instance contenant un contexte pour utiliser Vulkan
     * 
     * On y définit en particuliers le nom de l'application et du moteur Vulkan programmé/utilisé
     */
    vk::Instance instance;

    /**
     * @brief Pointeur sur un objet encapsulant l'extension de débogage fournit par Vulkan
     * 
     */
    std::unique_ptr<vulkan::debug::Utils> pt_dbg_utils{nullptr};
    /**
     * @brief Périphérique physique utilisé qui supporte l'utilisation de Vulkan.
     * 
     * Le périphérique physique est souvent une carte graphique, mais cela peut être aussi un CPU.
     * Concrétement, on manipule le périphérique physqiue viaun driver bas niveau
     * 
     */
    vk::PhysicalDevice physical_device;
    /**
     * @brief Périphérique logique qui permet, via un driver api, d"utiliser le périphérique physique
     * 
     */
    vk::Device         logical_device;

    /**
     * @brief Le pipeline spécifie le pipeline par lequel toutes commandes pour afficher ou calculer passe dans Vulkan
     * 
     * Un pipeline en Vulkan est une configuration fixe qui définit un chemin complet des données, depuis les données d'entrées jusqu'aux données de sortie
     * (vertex jusqu'au pixels pour le rendu par exemple). Puisque la configuration de calcul est connue à l'avance, cela permet au pipeline d'être extrêmement
     * performant. 
     * 
     * Notons que le pipeline aura la charge de compiler dans le binaire du GPU les shaders définis dans ses étapes.
     * 
     * Ici on va définir un simple pipeline permettant de faire un simple calcul avec Vulkan
     * 
     */
    vk::Pipeline       pipeline;
    vk::PipelineLayout pipeline_layout;
    vk::ShaderModule   compute_shader_module;

    /**
     * @brief Le buffer de commande utilisé pour enregistrer les commandes qui seront soumises à la queue de commande
     * 
     * Pour allouer les buffers de commande, on utilise un réservoire de commande (command pool)
     */
    vk::CommandPool    command_pool;
    vk::CommandBuffer  command_buffer;

    /**
     * @brief Les descripteurs représentent des ressources dans les shaders. 
     * 
     * Si il est possible de directement transférer des constantes du CPU vers le GPU, il n'est pas possible de directement utiliser des buffers, des textures,
     * etc, définis par le CPU directement par le GPU. Pour cela, on devra utiliser des ensembles de descripteurs qui seront le moyen de connecter des données
     * définies par le CPU au GPU.
     * 
     * Les descripteurs peuvent paraître très compliqués par rapport à d'autres alternatives utilisés par d'autres APIs graphiques. Ici, on va se concentrer
     * uniquement sur les descripteurs permettant d'utiliser des buffers.
     * 
     * Un descripteur peut être vu comme un pointeur (ou un hande) sur une ressource. Une ressource peut être un buffer ou une image et contient plusieurs
     * informations autre quels données mêmes. Par exemple, la taille du buffer, le type de sampler dans le cas d'une image. 
     * 
     * Un vk::DescriptorSet est une collection de descripteurs qui sont reliés ensemble. Vulkan ne vous permet pas de directement relier une unique ressoure à des shaders.
     * Les ressources sont regroupées dans les ensembles. Si vous souhaiter pouvoir les relier individuellement aux shaders, alors vous aurez besoin d'un 
     * vk::DescriptorSet pour chaque ressource. Cela n'est pas très efficace, et pire, cela ne marchera pas sur beaucoup de périphériques. Par exemple, en allant sur
     * la page https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxBoundDescriptorSets&platform=windows , vous verrez que beaucoup de périphériques ne peuvent gérer
     * que quatre ensemble de descripteurs pour un pipeline donné. Pour lever cette limitation, on va les regrouper selon leurs utilisations.
     * 
     * Un ensemble de descripteur numéroté 0 sera utilisé pour les ressources globales du moteur et sera liées une fois par image générée. 
     * Un ensemble de descripteur numéroté 1 sera utilisé pour des ressources définies par passage et liées une fois par passage dans le pipeline
     * Un ensemble de descripteur numéroté 2 sera utilisé pour les ressources matérielles
     * Un ensemble de descripteur numéroté 3 sera utilisé pour des ressources définiées par objet.
     * 
     * Les ensembles de descripteurs ont a être alloués directement par le moteur à partir d'un objet de type vk::DescriptorPool. L'allocation d'un ensemble 
     * de descripteurs se fera typiquement dans la VRAM du GPU. Une fois qu'un ensemble de descripteurs est alloué, on doit y écrire les liens (pointeurs) sur
     * les buffers/textures/etc. que vous utilisez. Une fois qu'un ensemble de descripteurs est lié et que vous utilisez une fonction telle que vk::CmdDraw, 
     * on ne peut plus modifier l'ensemble de descripteurs sauf si on a spécifier le drapeau vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind. 
     * Lorsqu'on définit un pool de descripteurs, on doit indiquer au pilote combien d'ensembles de descripteurs et combien de ressource on va utiliser. On définit
     * souvent des valeurs par défaut élevées, comme mille descripteurs, et lorsque le pool de descripteurs est plein, l'allocation va échouer avec une erreur. On peut
     * alors créer un nouveau pool pour contenur plus de descripteurs.
     * 
     * On peut optimiser l'allocation d'ensembles de descripteurs en interdisant la libération individuelle de descripteurs en ne définissant pas le drapeau
     * vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet. En effet, si ce drapeau est utilisé, on permet de désallouer individuellement les descripteurs.
     * Par exemple, pour des ensembles de descripteurs par image, n'utilisez pas ce drapeau et reinitialisez le pool directement plutôt que chaque descripteur
     * individuellement. Pour vos ensemble de descripteurs globaux, il est préférable de les allouer une seule fois et de les reutiliser d'une image à l'autre.
     * 
     * Une technique courante utilisée dans les moteurs de production est d'avoir un ensemble de pools de descripteurs par image où lorsqu'une allocation échoue,
     * on rajoute à cet ensemble un nouveau pool. Lorsque l'image est soumise et que vous avez bien attendu sa synchronisation, vous pouvez reinitialiser tous ces pools
     * de descripteur.
     * 
     * # Initialisation des descripteurs
     * 
     * Un ensemble de descripteurs fraîchement alloué n'est qu'un morceau de mémoire GPU sur lequels on doit faire pointer nos buffers. Pour cela, on utilise la
     * méthode updateDescriptorSets de notre périphérique logique qui prend en argument un tableau de vk::WriteDescriptorSet  qui chacun définit la ressource
     * utilisée (un buffer par exemple). 
     * 
     * Lorsqu'un ensemble de descripteurs est utilisé, il est immuable. Pour le mettre à jour, on doit attendre que les commandes aient finies d'être exécutées.
     * 
     * # Mise en lien des descripteurs
     * 
     * Lors de la constitution du pipeline vulkan, des "emplacements" spécifiques vont être créés vis à vis de ce que doivent utiliser les shaders. On doit spécifier
     * pour chacun de ces emplacements l'ensemble de descripteurs qui doit y être lié. 
     * 
     * # Disposition de l'ensemble des descripteurs
     * 
     * On peut décrire la forme des ensembles de descripteurs à l'aide de dispositions (layout). Par exemple, une disposition possible sera celle où un ensemble
     * de descripteurs permettra de lier deux buffers et une image. Lors de la création des pipelines ou de l'allocation d'un ensemble de descripteurs, on doit utiliser
     * ces dispositions. 
     * 
     */
    vk::DescriptorPool      descriptor_pool;
    vk::DescriptorSet       descriptor_set;
    vk::DescriptorSetLayout descriptor_set_layout;

    /**
     * @brief Gestion du buffer dans lequel on va stocker l'ensemble de mandelbrot
     * 
     * buffer est le tableau contigü 1D stocké sur la mémoire du périphérique 
     * buffer_memory est la partie du device qui va allouer/désallouer le buffer (sur la mémoire GPU ou la RAM selon ce qu'on aura définit dedans)
     */
    vk::Buffer              buffer;
    vk::DeviceMemory        buffer_memory;
    uint32_t                buffer_size;

    /**
     * @brief Le queue de commande
     * 
     * Afin d'exécuter les commandes sur un périphérique, les commandes doivent être soumises à une queue. 
     * Les commandes sont stockées dans un buffer de commande, qui sera envoyé à une queue.
     * 
     * Il existe différentes sortes de queues sur un périphérique. Certains types de queues supportent le graphisme,
     * d'autres non par exemple. Pour cette application, on veut au moins une queue qui supporte les opérations de calcul.
     * 
     */
    vk::Queue               queue;

    /**
     * @brief L'indice de la famille de queue utilisée
     * 
     * Les queues qui ont les mêmes capacités (par exemple pouvoir gérer le graphisme et des opérations de calcul) sont regroupées
     * en familles de queues.
     * 
     * Quand on soumet un buffer de commande, on doit spécifier sur quelle queue dans la famille on le soumet. 
     * Cette variable conserve une trace de l'indice de la queue dans cette famille.
     */
    uint32_t queue_family_index;    

    std::vector<const char *> enabled_layers{}, enabled_extensions{};


};
}