#include "RenderFrame.hpp"

#include <VulkanUtils.h>
#include <core/RenderContext.hpp>
#include <core/VertexData.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// resource stuff
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <stb_image.h>

typedef struct {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
} UniformBufferObject;

namespace TMP_RenderPass {
	VkRenderPass render_pass = VK_NULL_HANDLE;

	// each renderpass needs to be associated with a framebuffer.
	// They can be shared between passes if compatible, but can't be used at the same time
	// by two different frames in flight
	// let's start by creating one, then we can try one per frame, the one per <pass, frame>
	std::vector<VkFramebuffer> framebuffers;

	// depth buffer
	namespace {
		static VkImage depth_image;
		static VkDeviceMemory depth_image_memory;
		static VkImageView depth_image_view;
	}

	// one per attachment, in attachment order
	const VkClearValue clear_values[] = {
		(VkClearValue) { .color			= (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 1.0f }}	},
		(VkClearValue) { .depthStencil	= (VkClearDepthStencilValue){ 1.0f, 0 }				}
	};

	const int clear_values_count = sizeof(clear_values) / sizeof(VkClearValue);

	void create_renderpass(VkDevice device, VkFormat swapchain_image_format) {
		VkAttachmentDescription attachmentDescriptions[] = {
		(VkAttachmentDescription) { // color attachment
			.format = swapchain_image_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		(VkAttachmentDescription) { // depth attachment
			.format = VK_FORMAT_D32_SFLOAT, // TODO findDepthFormat()
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
		};
		const int attachmentDescriptionsCount = sizeof(attachmentDescriptions) / sizeof(VkAttachmentDescription);

		// subpasses
		const VkAttachmentReference attachmentColor = (VkAttachmentReference){
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		const VkAttachmentReference attachmentDepth = (VkAttachmentReference){
				.attachment = 1,
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};


		VkSubpassDescription subpass = { VK_PIPELINE_BIND_POINT_GRAPHICS };
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentColor;
		subpass.pDepthStencilAttachment = &attachmentDepth;
		subpass.inputAttachmentCount = 0;

		// TODO subpass dependencies https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Subpass-dependencies
		// TODO depth sbpass dependency https://vulkan-tutorial.com/Depth_buffering#page_Render-pass

		VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount = attachmentDescriptionsCount;
		renderPassInfo.pAttachments = attachmentDescriptions;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(device, &renderPassInfo, NULL, &render_pass) != VK_SUCCESS)
			CC_LOG(ERROR, "failed to create render pass");
	}

	void create_framebuffers(VkDevice device, const vkc::Swapchain *obj_swapchain) {
		
		VkExtent2D extents = obj_swapchain->get_extent();
		VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass = render_pass;
		framebufferInfo.width = extents.width;
		framebufferInfo.height = extents.height;
		framebufferInfo.layers = 1;

		int i = 0;
		framebuffers.resize(obj_swapchain->get_image_views().size());
		for (VkImageView swapchain_image_view : obj_swapchain->get_image_views())
		{
			const int attachmentsCount = 2;
			VkImageView attachments[] = {
				swapchain_image_view,
				depth_image_view
			};

			framebufferInfo.attachmentCount = attachmentsCount;
			framebufferInfo.pAttachments = attachments;
			if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]) != VK_SUCCESS)
				CC_LOG(ERROR, "failed to create framebuffer!");
			++i;
		}
	}

	void create_depth_resources(VkDevice device, vkc::RenderContext* obj_render_context) {
		VkExtent2D extent = obj_render_context->get_swapchain_extent();
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // TODO findDepthFormat()

		obj_render_context->create_image(
			extent.width,
			extent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&depth_image,
			&depth_image_memory
		);
		depth_image_view = obj_render_context->create_imge_view(
			depth_image,
			depthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT
		);
	}
}

namespace TMP_Pipeline {
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	// TODO create automatically from shader?
	VkDescriptorSetLayout descriptorSetLayout;

	// one of those per frame-in-flight
	VkDescriptorPool descriptor_pool;
	std::vector<VkDescriptorSet>	descriptor_sets;
	std::vector<VkBuffer>			uniform_buffers;
	std::vector<VkDeviceMemory>		uniform_buffers_memory;
	std::vector<void*>				uniform_buffers_mapped;

	// data
	// eventually we want to split image from sampler. It depends on how the VkWriteDescriptorSet is set up
	// see https://docs.vulkan.org/samples/latest/samples/api/separate_image_sampler/README.html
	// what we want is
	// - pipeline: VkSampler only
	// - application
	//    - VkImage
	//    - VkDeviceMemory
	//    - VkImageView
	// this requires different shaders too (separate setup uses two uniforms)
	// Default is probably split, but there are images that makes sense to keep unified
	// - shadow/light maps
	// - noise textures
	// - ...
	namespace {
		VkImage texture_image;
		VkDeviceMemory texture_image_memory;
		VkImageView texture_image_view;
		VkSampler texture_sampler;
	}

	VkShaderModule createShaderModule(VkDevice device, const char* code, const size_t codeSize) {
		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = codeSize;
		createInfo.pCode = (const uint32_t*)code;

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create shader module!");
		}

		return shaderModule;
	}

	void create_pipeline(VkDevice device, VkRenderPass render_pass) {
		std::vector<char> shaderCodeVert = TMP_VUlkanUtils::read_file_binary("res/shaders/shader_base.vert.spv");
		std::vector<char> shaderCodeFrag = TMP_VUlkanUtils::read_file_binary("res/shaders/shader_base.frag.spv");

		VkShaderModule shaderModuleVert = createShaderModule(device, shaderCodeVert.data(), shaderCodeVert.size());
		VkShaderModule shaderModuleFrag = createShaderModule(device, shaderCodeFrag.data(), shaderCodeFrag.size());

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaderModuleVert;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaderModuleFrag;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {
			vertShaderStageInfo,
			fragShaderStageInfo
		};

		// dynamic state
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		uint32_t dynamicStatesCount = sizeof(dynamicStates) / sizeof(VkDynamicState);

		VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicState.dynamicStateCount = dynamicStatesCount;
		dynamicState.pDynamicStates = dynamicStates;

		// vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertexInputInfo.vertexBindingDescriptionCount = vertexData_getBindingDescriptionsCount();
		vertexInputInfo.pVertexBindingDescriptions = vertexData_getBindingDescriptions();
		vertexInputInfo.vertexAttributeDescriptionCount = vertexData_getAttributeDescriptionsCount();
		vertexInputInfo.pVertexAttributeDescriptions = vertexData_getAttributeDescriptions();

		// input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport state
		VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizer.depthClampEnable = VK_FALSE;
		// rasterizer.rasterizerDiscardEnable = VK_FALSE; // this should be default false
		// rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // this should be fill by default
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		// multisampling
		VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;     // Optional
		multisampling.pSampleMask = NULL;     // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = (VkStencilOpState){}; // Optional
		depthStencil.back = (VkStencilOpState){}; // Optional

		// color blend - framebuffer
		VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;      // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;      // Optional

		// color blend - global constants
		VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// pipeline assembly
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipeline_layout) != VK_SUCCESS)
			CC_LOG(ERROR, "failed to create pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipeline_layout;

		pipelineInfo.renderPass = render_pass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1;              // Optional

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline) != VK_SUCCESS)
			CC_LOG(ERROR, "failed to create graphics pipeline");

		// cleanup
		vkDestroyShaderModule(device, shaderModuleFrag, NULL);
		vkDestroyShaderModule(device, shaderModuleVert, NULL);
	}

	void create_descriptor_set_layout(VkDevice device) {
		VkDescriptorSetLayoutBinding uboLayoutBinding = { 0 };
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = NULL; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding = { 0 };
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = NULL;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding bindings[] = {
			uboLayoutBinding,
			samplerLayoutBinding
		};
		static const uint32_t bindingsCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = bindingsCount;
		layoutInfo.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create descriptor set layout!");
		}
	}

	// TODO should this be automatically read from pipeline info?
	void create_descriptor_sets(VkDevice device, uint32_t swapchain_image_size) {
		// descriptor pool
		VkDescriptorPoolSize poolSize[] = {
		(VkDescriptorPoolSize) {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = swapchain_image_size
		},
		(VkDescriptorPoolSize) {
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = swapchain_image_size
		}
		};
		static const uint32_t poolSizeCount = sizeof(poolSize) / sizeof(VkDescriptorPoolSize);

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = poolSizeCount;
		poolInfo.pPoolSizes = poolSize;
		poolInfo.maxSets = swapchain_image_size;

		if (vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptor_pool) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create descriptor pool!");
		}

		std::vector<VkDescriptorSetLayout> layouts = {
			descriptorSetLayout,
			descriptorSetLayout,
			descriptorSetLayout
		};

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptor_pool;
		allocInfo.descriptorSetCount = swapchain_image_size;
		allocInfo.pSetLayouts = layouts.data();

		descriptor_sets.resize(swapchain_image_size);

		if (vkAllocateDescriptorSets(device, &allocInfo, descriptor_sets.data()) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < swapchain_image_size; i++) {
			VkDescriptorBufferInfo bufferInfo = { 0 };
			bufferInfo.buffer = uniform_buffers[i];
			bufferInfo.offset = 0;
			// TODO if UBO is application-specific, should we pass the size as parameter?
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo = { 0 };
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = texture_image_view;
			imageInfo.sampler = texture_sampler;

			VkWriteDescriptorSet descriptorWrites[] = {
				(VkWriteDescriptorSet) { // uniforms buffer
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = descriptor_sets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &bufferInfo
				},
				(VkWriteDescriptorSet) { // texture sampler
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = descriptor_sets[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &imageInfo
				}
			};
			static const uint32_t descriptorWritesCount = sizeof(descriptorWrites) / sizeof(VkWriteDescriptorSet);
			vkUpdateDescriptorSets(device, descriptorWritesCount, descriptorWrites, 0, NULL);
		}
	}

	void create_uniform_buffers(vkc::RenderContext* obj_render_context, VkDevice device, uint32_t swapchain_image_size) {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniform_buffers.resize(swapchain_image_size);
		uniform_buffers_memory.resize(swapchain_image_size);
		uniform_buffers_mapped.resize(swapchain_image_size);

		for (size_t i = 0; i < swapchain_image_size; ++i) {
			obj_render_context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffers[i], &uniform_buffers_memory[i]);
			vkMapMemory(device, uniform_buffers_memory[i], 0, bufferSize, 0, &uniform_buffers_mapped[i]);
		}
	}

	void create_texture_sampler(VkDevice device, vkc::RenderContext* obj_render_context) {
		VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE; // ATM we are not requesting/enabling it in physical/logical device
		samplerInfo.maxAnisotropy = obj_render_context->get_physical_device_properties().limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		// percentage-closer filtering (shadow mapping)
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		// mipmapping
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device, &samplerInfo, NULL, &texture_sampler) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create texture sampler!");
		}
	}
}

namespace TMP_Application {
	// ASSETS SECTION ==========================================================
	// C++ has no problem with enum sizes
	typedef enum : uint8_t {
		TEX_CHANNELS_NONE = 0x0,
		TEX_CHANNELS_GREY = 0x1,
		TEX_CHANNELS_GREY_A = 0x2,
		TEX_CHANNELS_RGB = 0x3,
		TEX_CHANNELS_RGB_A = 0x4
	} TexChannelTypes;

	typedef struct {
		std::vector<VertexData> vertices;
		std::vector<uint32_t> indices;
	} MeshData;

	typedef struct {
		uint16_t width;
		uint16_t height;
		uint8_t channelsCount;
		TexChannelTypes channels;
		std::vector<unsigned char> data;
	} TextureData;

	MeshData mesh_data;
	TextureData texture_data;
	// =========================================================================

	VkBuffer vertex_buffer;
	VkDeviceMemory vertexbuffer_memory;

	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;

	glm::mat4 perspective_projection;

	MeshData load_mesh(const char * path) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		std::string warn;

		assert(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path));

		MeshData ret;
		int n_indices = 0;
		// cunt indices (vertices will be the same number because we're cutting corners)
		for (const auto& shape : shapes)
			n_indices += shape.mesh.indices.size();

		ret.vertices.resize(n_indices);
		ret.indices.resize(n_indices);

		int i = 0;
		for(const auto& shape : shapes)
			for (const auto& index : shape.mesh.indices) {
				ret.indices[i] = i;
				ret.vertices[i].position = glm::vec3(
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				);
				ret.vertices[i].texCoords = glm::vec2(
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				);
				ret.vertices[i].color = glm::vec3(1.0f);
				++i;
			}

		return ret;
	}

	MeshData load_meshes(const char** paths) {
		return {}; // TODO
	}

	TextureData load_texture(const char* path, TexChannelTypes channels) {
		int texWidth = 0;
		int texHeight = 0;
		int texChannels = 0;
		stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, channels);
		assert(pixels != nullptr);

		// TODO get number of ACTUAL channels based on requested channels (`channels` parameter)
		size_t size = texWidth * texHeight * 4;

		TextureData ret;
		ret.width = (uint16_t)texWidth;
		ret.height = (uint16_t)texHeight;
		ret.channelsCount = (uint8_t)texChannels;
		ret.channels = channels;
		ret.data.resize(size);
		memcpy(ret.data.data(), pixels, size);

		stbi_image_free(pixels);
		return ret;
	}


	void createTextureImage(VkDevice device, vkc::RenderContext* obj_render_context) {
		texture_data = load_texture("res/textures/colormap.png", TEX_CHANNELS_RGB_A);
		VkDeviceSize imageSize = texture_data.width * texture_data.height * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		obj_render_context->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, texture_data.data.data(), imageSize);
		vkUnmapMemory(device, stagingBufferMemory);

		// // no free, we can't fit all our scene in GPU memory at the same time
		//res_tex_free(textureId);

		obj_render_context->create_image(
			texture_data.width, texture_data.height,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&TMP_Pipeline::texture_image,
			&TMP_Pipeline::texture_image_memory
		);

		obj_render_context->transition_image_layout(
			TMP_Pipeline::texture_image,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		obj_render_context->copy_buffer_to_image(stagingBuffer, TMP_Pipeline::texture_image, texture_data.width, texture_data.height);

		obj_render_context->transition_image_layout(
			TMP_Pipeline::texture_image,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		vkDestroyBuffer(device, stagingBuffer, NULL);
		vkFreeMemory(device, stagingBufferMemory, NULL);

		// view
		TMP_Pipeline::texture_image_view = obj_render_context->create_imge_view(
			TMP_Pipeline::texture_image,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}

	// createIndexBuffers
	// createVertexBuffers
	void createModelBuffers(VkDevice device, vkc::RenderContext* obj_render_context) {
		// load mesh data
		mesh_data = load_mesh("res/models/viking_room.obj");

		// we could use a single staging buffer here, but then we would need to sync them.
		// alternatively, create a RenderContext::copyBuffers() that handles it appropriately
		// (avoiding to create and destroy the same staring buffer seems like a good idea in any case,
		// just a metter of figuring out how)
		// vertices ============================================================
		{
			// TODO FIXME this won't work if we don't use indices.
			// but we can't just sizeof(vertices), since now vertices is now a pointer variable
			VkDeviceSize bufferSize = sizeof(mesh_data.vertices[0]) * mesh_data.vertices.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			obj_render_context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

			// map memory
			void* data;
			vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, mesh_data.vertices.data(), (size_t)bufferSize);
			vkUnmapMemory(device, stagingBufferMemory);

			obj_render_context->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&vertex_buffer,
				&vertexbuffer_memory
			);

			obj_render_context->copyBuffer(stagingBuffer, vertex_buffer, bufferSize);

			vkDestroyBuffer(device, stagingBuffer, NULL);
			vkFreeMemory(device, stagingBufferMemory, NULL);
		}


		// indices  ============================================================
		{
			// TODO FIXME this won't work if we don't use indices.
			// but we can't just sizeof(indices), since now indices is now a pointer variable
			VkDeviceSize bufferSize = sizeof(mesh_data.indices[0]) * mesh_data.indices.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			obj_render_context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

			// map memory
			void* data;
			vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, mesh_data.indices.data(), (size_t)bufferSize);
			vkUnmapMemory(device, stagingBufferMemory);

			obj_render_context->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&index_buffer,
				&index_buffer_memory
			);

			obj_render_context->copyBuffer(stagingBuffer, index_buffer, bufferSize);

			vkDestroyBuffer(device, stagingBuffer, NULL);
			vkFreeMemory(device, stagingBufferMemory, NULL);
		}
	}

	static float x = 0.0f;
	void updateUniformBuffer(uint32_t currentFrame) {
		// TODO calculate deltaTime
		const float time = 1 / 60.0f;
		const float offset = 1.0f;
		x += time;


		const glm::vec3 dir_up = (glm::vec3){ 0, 0, 1 };
		const glm::vec3 pos_camera = glm::rotate(glm::mat4(1.0f), x, dir_up) * (glm::vec4) { 2, 2, 2, 1 };
		const glm::vec3 pos_target = (glm::vec3){ 0, 0, 0 };

		glm::mat4 m = glm::mat4(1.0f);
		UniformBufferObject ubo = (UniformBufferObject){
			.model = m,
			.view = glm::lookAt(pos_camera, pos_target, dir_up),
			.proj = perspective_projection
		};

		// invert up axis
		ubo.proj[1][1] *= -1;
		memcpy(TMP_Pipeline::uniform_buffers_mapped[currentFrame], &ubo, sizeof(ubo));
	}

	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t imageIndex, VkExtent2D TMP_extent) {
		const VkDescriptorSet* descriptor_set = &TMP_Pipeline::descriptor_sets[imageIndex];

		VkBuffer vertexBuffers[] = { vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
			command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TMP_Pipeline::pipeline_layout,
			0, 1, descriptor_set,
			0, NULL
		);
		vkCmdDrawIndexed(command_buffer, mesh_data.indices.size(), 1, 0, 0, 0);
	}
}

namespace vkc {
	RenderFrame::RenderFrame(
		VkDevice device,
		RenderContext* render_context,
		VkCommandBuffer command_buffer
	) {
		m_render_context = render_context;
		m_device = device;

		m_command_buffer = command_buffer;

		// synch objects
		VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (
			vkCreateSemaphore(device, &semaphoreInfo, NULL, &m_semaphore_image_available) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, NULL, &m_semaphore_render_finished) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, NULL, &m_fence_in_flight) != VK_SUCCESS
			) {
			CC_LOG(ERROR, "failed to create sync objects");
		}

		// stupid hack; All of this should trigger one time only
		if (TMP_RenderPass::render_pass == VK_NULL_HANDLE)
		{
			// Temporary data (will be moved eventually in proper place)

			TMP_RenderPass::create_depth_resources(device, render_context);
			TMP_RenderPass::create_renderpass(
				device,
				render_context->get_swapchain_image_format()
			);
			TMP_RenderPass::create_framebuffers(
				device,
				render_context->get_obj_swapchain()
			);

			TMP_Pipeline::create_descriptor_set_layout(device);
			TMP_Pipeline::create_pipeline(device, TMP_RenderPass::render_pass);
			// weird that we have to create the buffers before the descriptors
			TMP_Pipeline::create_uniform_buffers(
				render_context,
				device,
				render_context->get_num_render_frames()
			);
			
			TMP_Pipeline::create_texture_sampler(device, render_context);

			TMP_Application::createModelBuffers(device, render_context);
			TMP_Application::createTextureImage(device, render_context);

			// descriptors need the actual data to be created
			// raises a lot of question about how to handle multiple Drawcalls
			TMP_Pipeline::create_descriptor_sets(
				device,
				render_context->get_num_render_frames()
			);
		}
	}

	RenderFrame::~RenderFrame() {
		// stupid hack; All of this should trigger one time only
		if (TMP_RenderPass::render_pass != VK_NULL_HANDLE)
		{
			// TMP application
			{
				vkDestroyBuffer(m_device, TMP_Application::vertex_buffer, NULL);
				vkDestroyBuffer(m_device, TMP_Application::index_buffer, NULL);
				vkFreeMemory(m_device, TMP_Application::index_buffer_memory, NULL);
				vkFreeMemory(m_device, TMP_Application::vertexbuffer_memory, NULL);
			}

			// TMP pipeline
			{
				vkDestroyPipelineLayout(m_device, TMP_Pipeline::pipeline_layout, NULL);
				vkDestroyPipeline(m_device, TMP_Pipeline::pipeline, NULL);
				vkDestroyDescriptorSetLayout(m_device, TMP_Pipeline::descriptorSetLayout, NULL);

				// looks like descriptor sets do not need to be cleaned up at the end of the application
				/*vkFreeDescriptorSets(
					m_device,
					TMP_Pipeline::descriptor_pool,
					TMP_Pipeline::descriptor_sets.size(),
					TMP_Pipeline::descriptor_sets.data()
				);*/

				vkDestroyDescriptorPool(m_device, TMP_Pipeline::descriptor_pool, NULL);
				for (auto& handle : TMP_Pipeline::uniform_buffers)
					vkDestroyBuffer(m_device, handle, NULL);
				for (auto& handle : TMP_Pipeline::uniform_buffers_memory)
					vkFreeMemory(m_device, handle, NULL);

				vkDestroyImageView(m_device, TMP_Pipeline::texture_image_view, NULL);
				vkDestroyImage(m_device, TMP_Pipeline::texture_image, NULL);
				vkFreeMemory(m_device, TMP_Pipeline::texture_image_memory, NULL);
				vkDestroySampler(m_device, TMP_Pipeline::texture_sampler, NULL);
				
			}
			// TMP renderpass
			{
				vkDestroyImageView(m_device, TMP_RenderPass::depth_image_view, NULL);
				vkDestroyImage(m_device, TMP_RenderPass::depth_image, NULL);
				vkFreeMemory(m_device, TMP_RenderPass::depth_image_memory, NULL);

				for (auto& handle : TMP_RenderPass::framebuffers)
					vkDestroyFramebuffer(m_device, handle, NULL);

				vkDestroyRenderPass(m_device, TMP_RenderPass::render_pass, NULL);
				TMP_RenderPass::render_pass = VK_NULL_HANDLE;
			}
		}
		vkDestroyFence(m_device, m_fence_in_flight, NULL);
		vkDestroySemaphore(m_device, m_semaphore_render_finished, NULL);
		vkDestroySemaphore(m_device, m_semaphore_image_available, NULL);
	}

	void RenderFrame::render(
		VkSwapchainKHR swapchain,
		VkQueue queue_render,
		VkQueue queue_present,
		uint32_t frame_index,
		VkExtent2D swapchain_extent
	) {
		// 1. [frame]	begin command buffer
		// 2. [pass]		begin renderpass
		// 3. [shdr	]		bind pipeline
		// 4. [pass	]			draw calls (+ uniforms)
		// 5. [pass	]		end renderpass
		// 6. [frame]	end command buffer
		// 5. [ctx	]	submit to render queue
		// 6. [ctx	]	submit to present queue



		vkWaitForFences(m_device, 1, &m_fence_in_flight, VK_TRUE, UINT64_MAX);

		VkResult resultNextImage = vkAcquireNextImageKHR(m_device, swapchain, UINT64_MAX, m_semaphore_image_available, VK_NULL_HANDLE, &frame_index);
		if (resultNextImage == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_render_context->recreate_swapchain();
			return;
		}

		else if (resultNextImage > VK_SUCCESS)
			CC_LOG(LOG, "[VkResult %d] %s", resultNextImage, string_VkResult(resultNextImage));
		else if (resultNextImage < VK_SUCCESS)
			CC_LOG(ERROR, "[VkResult %d] failed to present swapchain: %s", resultNextImage, string_VkResult(resultNextImage));


		float fow = (float)swapchain_extent.width / swapchain_extent.height;
		TMP_Application::perspective_projection = glm::perspective(glm::radians(45.0f), fow, 0.1f, 10.0f);

		// this will be executed reading the uniform data form a queue of DrawCalls
		TMP_Application::updateUniformBuffer(frame_index);

		vkResetFences(m_device, 1, &m_fence_in_flight);

		vkResetCommandBuffer(m_command_buffer, 0);
		
		// cmd buffer =========================================================
		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = 0;               // Optional
		beginInfo.pInheritanceInfo = NULL; // Optional


		if (vkBeginCommandBuffer(m_command_buffer, &beginInfo) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to begin recording command buffer!");
		}

		// renderpass =========================================================
		VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassInfo.renderPass = TMP_RenderPass::render_pass;
		renderPassInfo.framebuffer = TMP_RenderPass::framebuffers[frame_index]; // FIXME one per in-flight-frame
		renderPassInfo.renderArea.offset = (VkOffset2D){
			.x = 0,
			.y = 0
		};
		renderPassInfo.renderArea.extent = swapchain_extent;
		renderPassInfo.clearValueCount = TMP_RenderPass::clear_values_count;
		renderPassInfo.pClearValues = TMP_RenderPass::clear_values;

		vkCmdBeginRenderPass(m_command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// ====================================================================

		vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TMP_Pipeline::pipeline);
		VkViewport viewport = { };
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = swapchain_extent.width;
		viewport.height = swapchain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);

		VkRect2D scissor = { };
		scissor.offset = (VkOffset2D){
			.x = 0,
			.y = 0
		};
		scissor.extent = swapchain_extent;
		vkCmdSetScissor(m_command_buffer, 0, 1, &scissor);

		// this will be executed reading the uniform data form a queue of DrawCalls
		TMP_Application::recordCommandBuffer(m_command_buffer, frame_index, swapchain_extent);


		vkCmdEndRenderPass(m_command_buffer);

		if (vkEndCommandBuffer(m_command_buffer) != VK_SUCCESS)
			CC_LOG(ERROR, "failed to record command buffer");


		// submit queues
		VkSubmitInfo submitInfo = { };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_semaphore_image_available };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_command_buffer;

		VkSemaphore signalSemaphores[] = { m_semaphore_render_finished };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(queue_render, 1, &submitInfo, m_fence_in_flight) != VK_SUCCESS)
			CC_LOG(ERROR, "failed to submit draw command buffer");

		VkPresentInfoKHR presentInfo = { };
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &frame_index;
		presentInfo.pResults = NULL; // Optional

		VkResult presentResult = vkQueuePresentKHR(queue_present, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
			m_render_context->recreate_swapchain();

		if (presentResult > VK_SUCCESS)
			CC_LOG(LOG, "[VkResult %d] %s", presentResult, string_VkResult(presentResult));
		else if (presentResult < VK_SUCCESS)
			CC_LOG(ERROR, "[VkResult %d] failed to present swapchain: ", presentResult, string_VkResult(presentResult));
	}

	void RenderFrame::handle_swapchain_recreation() {
		vkDeviceWaitIdle(m_device);

		handle_swapchain_destruction();

		TMP_RenderPass::create_depth_resources(m_device, m_render_context);
		TMP_RenderPass::create_framebuffers(m_device, m_render_context->get_obj_swapchain());
	}

	void RenderFrame::handle_swapchain_destruction() {
		vkDestroyImageView(m_device, TMP_RenderPass::depth_image_view, NULL);
		vkDestroyImage(m_device, TMP_RenderPass::depth_image, NULL);
		vkFreeMemory(m_device, TMP_RenderPass::depth_image_memory, NULL);

		for (uint32_t i = 0; i < TMP_RenderPass::framebuffers.size(); ++i)
			vkDestroyFramebuffer(m_device, TMP_RenderPass::framebuffers[i], NULL);
	}
}