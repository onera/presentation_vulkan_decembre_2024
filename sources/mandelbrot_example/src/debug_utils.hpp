#ifndef _VULKAN_DEBUG_UTILS_HPP_
#define _VULKAN_DEBUG_UTILS_HPP_
#include <vulkan/vulkan.hpp>

namespace vulkan::debug
{
class Utils
{
public:
    constexpr static auto needed_layer_name = "VK_LAYER_KHRONOS_validation";
    Utils( vk::Instance t_instance );
    Utils( Utils const& ) = delete;
    Utils( Utils     && ) = delete;
    ~Utils();

    Utils& operator = ( Utils const& ) = delete;
    Utils& operator = ( Utils     && ) = delete;

    //@name Fonctions général pour le déboguage
    //@{
    /**
     * @brief Vérifie si la plateforme Vulkan utilisée supporte un mode de déboguage dans les extensions proposées
     * 
     * @return true   Retourne vrai si l'extension de déboguage est supportée
     * @return false  Retourne faux si l'extension de déboguage n'est pas supportée
     */
    static bool check_extension();
    //@}

    //@name Utilitaires pour créer un messager en cas d'erreurs, de warning ou de mesure de performance
    //@{
    /**
     * @brief Créer un messager pour l'instance afin de déboguer jusqu'à l'appel à destroy_messenger
     * 
     */
    void create_messenger();
    /**
     * @brief Détruit un messager de déboguage.
     * 
     */
    void destroy_messenger();
    //@}

    //@name Fonctions permettant de définir des régions et des étiquettes dans un buffer de commande
    //@{
    /**
     * @brief Insérer une commande d'affichage d'une étiquette et le début d'une région dans un buffer de commande
     * 
     * @param command_buffer Le buffer de commande
     * @param label_name     Le nom donné à l'étiquette
     * @param color          La couleur appliquée à l'étiquette pour les utilitaires de déboguage vulkan.
     */
    void command_begin_label ( vk::CommandBuffer command_buffer, char const* label_name, std::vector<float> const& color );
    /**
     * @brief Insère au sein d'un buffer de commande l'affichage d'une étiquette
     * 
     * @param command_buffer Le buffer de commande
     * @param label_name     Le nom donné à l'étiquette
     * @param color          La couleur appliquée à l'étiquette pour les utilitaires de déboguage vulkan.
     */
    void command_insert_label( vk::CommandBuffer command_buffer, char const* label_name, std::vector<float> const& color );
    /**
     * @brief Insère la fin d'une région dont le début a été défini avec le dernier appel au command_begin_label
     * 
     * Il est important de s'assurer que le nombre d'appel de command_end_label soit égal au nombre d'appel à command_begin_label afin de bien délimiter les régions
     * affichée dans le buffer de commande
     * 
     * @param command_buffer 
     */
    void command_end_label( vk::CommandBuffer command_buffer );
    //@}

    //@name Fonctions permettant de définir des régions et des étiquettes qui s'afficheront lors de l'exécution d'une queue
    //@{
    /**
     * @brief Rajoute une nouvelle zone avec une étiquette dans une queue vulkan
     * 
     * @param queue       La queue vulkan
     * @param label_name  Nom donnée à la zone
     * @param color       La couleur à afficher pour cette zone
     */
    void queue_begin_label( vk::Queue queue, char const* label_name, std::vector<float> const &color);

    /**
     * @brief Rajoute une étiquette dans une queue vulkan
     * 
     * @param queue       La queue vulkan
     * @param label_name  Le nom donnée à l'étiquette
     * @param color       La couleur donnée à l'étiquette (pour une utilitaire de débogage Vulkan)
     */
    void queue_insert_label(vk::Queue queue, char const*label_name, std::vector<float> const &color);

    /**
     * @brief Définie la fin d'une zone de débogage pour une queue vulkan
     * 
     * @param queue la queue vulkan
     * 
     *  La fin de la zone est celle dont le début a été défini par le dernier appel à queue_begin_label
     * 
     *  Attention : il est important de s'assurer qu'il y ait autant d'appels de queue_begin_label et de queue_end_label.
     */
    void queue_end_label(vk::Queue queue);

    /**
     * @brief Défini un nom à un objet Vulkan
     * 
     * @param device        Une device de Vulkan
     * @param object_type   Le type d'objet à nommer
     * @param object_handle L'adresse de l'objet à nommer
     * @param object_name   Le nom que l'on veut donner à l'objet
     */
    void set_object_name( vk::Device device, vk::ObjectType object_type, uint64_t object_handle, char const* object_name);

    /**
     * @brief Rajoute un shader avec des étiquettes données automatiquement par rapport au nom de fichier contenant le shader
     * 
     * @param device 
     * @param file 
     * @param stage 
     * @return vk::PipelineShaderStageCreateInfo 
     */
    vk::PipelineShaderStageCreateInfo load_shader( vk::Device device, std::string const& file, vk::ShaderStageFlagBits stage );
    //@}
private:
    vk::Instance m_instance{};
    VkDebugUtilsMessengerEXT messenger;
    PFN_vkCreateDebugUtilsMessengerEXT  m_create_messenger{nullptr};
    PFN_vkDestroyDebugUtilsMessengerEXT m_destroy_messenger{nullptr};
    PFN_vkSubmitDebugUtilsMessageEXT    m_submit_message{nullptr};
    PFN_vkCmdBeginDebugUtilsLabelEXT    m_command_begin_label{nullptr};
    PFN_vkCmdInsertDebugUtilsLabelEXT   m_command_insert_label{nullptr};
    PFN_vkCmdEndDebugUtilsLabelEXT      m_command_end_label{nullptr};
    PFN_vkQueueBeginDebugUtilsLabelEXT  m_queue_begin_label{nullptr};
    PFN_vkQueueInsertDebugUtilsLabelEXT m_queue_insert_label{nullptr};
    PFN_vkQueueEndDebugUtilsLabelEXT    m_queue_end_label{nullptr};
    PFN_vkSetDebugUtilsObjectNameEXT    m_set_object_name{nullptr};
};
}

#endif