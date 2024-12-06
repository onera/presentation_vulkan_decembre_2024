#include <iostream>
#include <unordered_map>
#include <string>
using namespace std::string_literals;
#include "debug_utils.hpp"
#include "ansi.hpp"

namespace vulkan::debug
{
namespace {
std::string debug_annotation_object_to_string(VkObjectType type)
{
    std::string stype;
    switch(type)
    {
    case VK_OBJECT_TYPE_BUFFER:
        stype = "Buffer"s;
        break;
    case VK_OBJECT_TYPE_BUFFER_VIEW:
        stype = "Buffer view"s;
        break;
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
        stype = "Command Buffer"s;
        break;
    case VK_OBJECT_TYPE_COMMAND_POOL:
        stype = "Command Pool"s;
        break;
    case VK_OBJECT_TYPE_DEVICE:
        stype = "Device"s;
        break;
    case VK_OBJECT_TYPE_QUEUE:
        stype = "Queue"s;
        break;
    default:
        stype = "(Unknown)"s;
        break;
    }
    return stype;
}
// ....................................................................................................................
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data)
{
    std::string prefix;
    std::string message;

    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    { prefix = std::string(ansi::BBlue) + std::string(ansi::Yellow) + " Warning "s; }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    { prefix = std::string(ansi::BWhite) + std::string(ansi::Black) + " Verbose "s; }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    { prefix = std::string(ansi::BGreen) + std::string(ansi::Black) + " Info "s; }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    { prefix = std::string(ansi::BRed) + std::string(ansi::Black) + " Error "s; }

    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        prefix += "general "s; 
    else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        prefix += "performance "s;
	else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		prefix += "validation "s;
    prefix += std::string(ansi::Normal);

    message = prefix + " - message n°"s + std::to_string(callback_data->messageIdNumber);
    message += " : "s + std::string(callback_data->pMessage);

    if (callback_data->objectCount > 0)
    {
        message += "\n\tObjects - "s + std::to_string(callback_data->objectCount) + "\n"s;
        for (uint32_t object = 0; object < callback_data->objectCount; ++object )
        {
            message += "\t\tObject["s + std::to_string(object) + "] - Type "s + debug_annotation_object_to_string(callback_data->pObjects[object].objectType);
            message += ", Value "s + std::to_string(callback_data->pObjects[object].objectHandle);
            message += ", Name \""s + std::string(callback_data->pObjects[object].pObjectName) + "\"\n"; 
        }
    }

    if (callback_data->cmdBufLabelCount > 0)
    {
        message += "\n\tEtiquettes Command Buffer - "s + std::to_string(callback_data->cmdBufLabelCount) + "\n"s;
        for (uint32_t label = 0; label < callback_data->cmdBufLabelCount; ++label)
        {
            message += "\t\tLabel["s + std::to_string(label) + "] - "s + std::string(callback_data->pCmdBufLabels[label].pLabelName) +
                       " { "s + std::to_string(callback_data->pCmdBufLabels[label].color[0]) + ", "s +
                       std::to_string(callback_data->pCmdBufLabels[label].color[1]) + ", "s +
                       std::to_string(callback_data->pCmdBufLabels[label].color[2]) + ", "s + 
                       std::to_string(callback_data->pCmdBufLabels[label].color[3]) + " }\n"s;
        }
    }
    std::cout << message << std::fflush;
    return vk::False;
}
}// End anonymous namespace
// ====================================================================================================================
Utils::Utils( vk::Instance t_instance )
	:	m_instance(t_instance)
{
	PFN_vkVoidFunction temp_fp{nullptr};

	temp_fp = m_instance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
	if( !temp_fp ) throw "Impossible de charger vkCreateDebugUtilsMessengerEXT"; // check shouldn't be necessary (based on spec)
	this->m_create_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT" );
	if( !temp_fp ) throw "Impossible de charger vkDestroyDebugUtilsMessengerEXT"; // check shouldn't be necessary (based on spec)
	this->m_destroy_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( temp_fp );

    temp_fp = m_instance.getProcAddr("vkSubmitDebugUtilsMessageEXT" );
	if( !temp_fp ) throw "Impossible de charger vkSubmitDebugUtilsMessageEXT"; // check shouldn't be necessary (based on spec)
	this->m_submit_message = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkCmdBeginDebugUtilsLabelEXT");
	if( !temp_fp ) throw "Impossible de charger vkCmdBeginDebugUtilsLabelEXT"; // check shouldn't be necessary (based on spec)
	this->m_command_begin_label = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkCmdInsertDebugUtilsLabelEXT");
	if( !temp_fp ) throw "Impossible de charger vkCmdInsertDebugUtilsLabelEXT"; // check shouldn't be necessary (based on spec)
	this->m_command_insert_label = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkCmdEndDebugUtilsLabelEXT");
	if( !temp_fp ) throw "Impossible de charger vkCmdEndDebugUtilsLabelEXT"; // check shouldn't be necessary (based on spec)
	this->m_command_end_label = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkQueueBeginDebugUtilsLabelEXT");
	if( !temp_fp ) throw "Impossible de charger vkQueueBeginDebugUtilsLabelEXT"; // check shouldn't be necessary (based on spec)
	this->m_queue_begin_label = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkQueueInsertDebugUtilsLabelEXT");
	if( !temp_fp ) throw "Impossible de charger vkQueueInsertDebugUtilsLabelEXT"; // check shouldn't be necessary (based on spec)
	this->m_queue_insert_label = reinterpret_cast<PFN_vkQueueInsertDebugUtilsLabelEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkQueueEndDebugUtilsLabelEXT");
	if( !temp_fp ) throw "Impossible de charger vkQueueEndDebugUtilsLabelEXT"; // check shouldn't be necessary (based on spec)
	this->m_queue_end_label = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>( temp_fp );

	temp_fp = m_instance.getProcAddr("vkSetDebugUtilsObjectNameEXT");
	if( !temp_fp ) throw "Impossible de charger vkSetDebugUtilsObjectNameEXT"; // check shouldn't be necessary (based on spec)
	this->m_set_object_name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>( temp_fp );
}
// --------------------------------------------------------------------------------------------------------------------
Utils::~Utils()
{}
// ====================================================================================================================
void Utils::create_messenger()
{
	vk::DebugUtilsMessengerCreateInfoEXT create_info;
	create_info = vk::DebugUtilsMessengerCreateInfoEXT()
		.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError   | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
		.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral    | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation )
		.setPfnUserCallback(debug_utils_messenger_callback)
		.setPUserData(nullptr);
	auto vk_create_info = static_cast<VkDebugUtilsMessengerCreateInfoEXT>(create_info);
	this->m_create_messenger(m_instance, &vk_create_info, nullptr, &messenger);
}
// --------------------------------------------------------------------------------------------------------------------
void Utils::destroy_messenger()
{
	this->m_destroy_messenger(m_instance, messenger, nullptr);
}
// ====================================================================================================================
bool Utils::check_extension()
{
    auto enabled_instance_extensions = vk::enumerateInstanceExtensionProperties();
	for (auto enabled_extension : enabled_instance_extensions)
		if (strcmp(enabled_extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) return true;
    std::cout << ansi::BYellow << ansi::Blue << " Attention : " << ansi::Normal
              << "L'extension requise " << VK_EXT_DEBUG_UTILS_EXTENSION_NAME << " n'est pas supportée ou disponible, "
              << "pas de débogue possible sur cette plateforme et ce mode de production" << std::endl;
    std::cout << "Assurez vous de compiler le code en mode débogue et/ou activer la couche de validation." << std::endl;
    return false;
}
// ====================================================================================================================
VKAPI_ATTR void VKAPI_CALL
Utils::command_begin_label( vk::CommandBuffer command_buffer, char const* label_name, std::vector<float> const& color )
{
    VkDebugUtilsLabelEXT label{ .pLabelName = label_name, .color = { color[0], color[1], color[2], color[3] } };
	m_command_begin_label(command_buffer, &label );
}
// --------------------------------------------------------------------------------------------------------------------
VKAPI_ATTR void VKAPI_CALL
Utils::command_insert_label( vk::CommandBuffer command_buffer, char const* label_name, std::vector<float> const& color )
{
    VkDebugUtilsLabelEXT label{ .pLabelName = label_name, .color = { color[0], color[1], color[2], color[3] } };
	m_command_insert_label( command_buffer, &label );
}
// --------------------------------------------------------------------------------------------------------------------
VKAPI_ATTR void VKAPI_CALL
Utils::command_end_label( vk::CommandBuffer command_buffer )
{
	m_command_end_label( command_buffer );
}
// ====================================================================================================================
VKAPI_ATTR void VKAPI_CALL 
Utils::queue_begin_label( vk::Queue queue, char const* label_name, std::vector<float> const &color)
{
	VkDebugUtilsLabelEXT label = { .pLabelName = label_name, .color = { color[0], color[1], color[2], color[3] } };
	m_queue_begin_label( queue, &label );
}
// --------------------------------------------------------------------------------------------------------------------
VKAPI_ATTR void VKAPI_CALL Utils::queue_insert_label(vk::Queue queue, char const*label_name, std::vector<float> const &color)
{
    VkDebugUtilsLabelEXT label = { .pLabelName = label_name, .color = { color[0], color[1], color[2], color[3] } };
	m_queue_insert_label( queue, &label );
}
// --------------------------------------------------------------------------------------------------------------------
VKAPI_ATTR void VKAPI_CALL Utils::queue_end_label(vk::Queue queue)
{
	m_queue_end_label(queue);
}
// ====================================================================================================================
VKAPI_ATTR void VKAPI_CALL 
Utils::set_object_name( vk::Device device, vk::ObjectType object_type, uint64_t object_handle, char const* object_name)
{
    VkDebugUtilsObjectNameInfoEXT name_info = { .objectType  = static_cast<VkObjectType>(object_type), .objectHandle = object_handle, 
                                                .pObjectName =  object_name };
	m_set_object_name( device, &name_info );
}
// --------------------------------------------------------------------------------------------------------------------
VKAPI_ATTR vk::PipelineShaderStageCreateInfo VKAPI_CALL Utils::load_shader( vk::Device device, std::string const& file, vk::ShaderStageFlagBits stage )
{
    /*
	// Note: this can be reworked once offline compilation for GLSL shaders is added

	// Default to GLSL
	std::string               shader_folder{"glsl"};
	std::string               shader_extension{""};
	vkb::ShaderSourceLanguage src_language = vkb::ShaderSourceLanguage::GLSL;

	if (get_shading_language() == vkb::ShadingLanguage::HLSL)
	{
		shader_folder = "hlsl";
		// HLSL shaders are offline compiled to SPIR-V, so source is SPV
		src_language     = vkb::ShaderSourceLanguage::SPV;
		shader_extension = ".spv";
	}

	std::string shader_file_name = "debug_utils/" + shader_folder + "/" + file;
    */
	VkPipelineShaderStageCreateInfo shader_stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, 
                                                     .stage = static_cast<VkShaderStageFlagBits>(stage) };
    /*
	shader_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage                           = stage;
	shader_stage.module                          = vkb::load_shader((shader_file_name + shader_extension).c_str(), get_device().get_handle(), stage, src_language);
	shader_stage.pName                           = "main";
	assert(shader_stage.module != VK_NULL_HANDLE);
	shader_modules.push_back(shader_stage.module);

	if (debug_utils_supported)
	{
		// Name the shader (by file name)
		set_object_name(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t) shader_stage.module, static_cast<std::string>("Shader " + file).c_str());

		if (get_shading_language() == vkb::ShadingLanguage::HLSL)
		{
			// Plain text HLSL file names are suffixed with .hlsl
			shader_file_name = shader_file_name + ".hlsl";
		}

		std::vector<uint8_t> buffer = vkb::fs::read_shader_binary(shader_file_name);

		// Pass the source GLSL shader code via an object tag
		VkDebugUtilsObjectTagInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT};
		info.objectType                   = VK_OBJECT_TYPE_SHADER_MODULE;
		info.objectHandle                 = (uint64_t) shader_stage.module;
		info.tagName                      = 1;
		info.tagSize                      = buffer.size() * sizeof(uint8_t);
		info.pTag                         = buffer.data();
		vkSetDebugUtilsObjectTagEXT(get_device().get_handle(), &info);
	}
    */
	return shader_stage;

}

/*
 * Name and tag some Vulkan objects
 * All objects named in this function will appear with the those names in a debugging tool
 */
/*void DebugUtils::debug_name_objects()
{
	if (!debug_utils_supported)
	{
		return;
	}
	set_object_name(VK_OBJECT_TYPE_BUFFER, (uint64_t) uniform_buffers.matrices->get_handle(), "Matrices uniform buffer");

	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.skysphere, "Skysphere pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.composition, "Skysphere pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.sphere, "Sphere rendering pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.bloom[0], "Horizontal bloom filter pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.bloom[1], "Vertical bloom filter pipeline");

	set_object_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t) pipeline_layouts.models, "Model rendering pipeline layout");
	set_object_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t) pipeline_layouts.composition, "Composition pass pipeline layout");
	set_object_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t) pipeline_layouts.bloom_filter, "Bloom filter pipeline layout");

	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.sphere, "Sphere model descriptor set");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.skysphere, "Sky sphere model descriptor set");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.composition, "Composition pass descriptor set");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.bloom_filter, "Bloom filter descriptor set");

	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) descriptor_set_layouts.models, "Model rendering descriptor set layout");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) descriptor_set_layouts.composition, "Composition pass set layout");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) descriptor_set_layouts.bloom_filter, "Bloom filter descriptor set layout");

	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) textures.skysphere.image->get_vk_image().get_handle(), "Sky sphere texture");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.depth.image, "Offscreen pass depth image");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.depth.image, "Offscreen pass depth image");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.color[0].image, "Offscreen pass color image 0");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.color[1].image, "Offscreen pass color image 1");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) filter_pass.color[0].image, "Bloom filter pass color image");

	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) depth_stencil.image, "Base depth/stencil image");
	for (size_t i = 0; i < swapchain_buffers.size(); i++)
	{
		std::string name = "Swapchain image" + std::to_string(i);
		set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) swapchain_buffers[i].image, name.c_str());
	}

	set_object_name(VK_OBJECT_TYPE_SAMPLER, (uint64_t) offscreen.sampler, "Offscreen pass sampler");
	set_object_name(VK_OBJECT_TYPE_SAMPLER, (uint64_t) filter_pass.sampler, "Bloom filter pass sampler");

	set_object_name(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t) offscreen.render_pass, "Offscreen pass render pass");
	set_object_name(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t) filter_pass.render_pass, "Bloom filter pass render pass");
}
*/
}