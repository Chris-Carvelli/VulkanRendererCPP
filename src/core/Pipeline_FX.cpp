#include "Pipeline_FX.hpp"

#include <VulkanUtils.h>
#include <core/RenderContext.hpp>
#include <core/DrawCall.hpp>
#include <core/VertexData.h>

namespace vkc {
	Pipeline_FX::Pipeline_FX(
		VkDevice handle_device,
		vkc::RenderContext* obj_render_context,
		VkRenderPass handle_render_pass,
		const char *vert_path,
		const char *frag_path,
		VkCullModeFlags face_culling_mode // TODO config struct
	)
		: m_handle_device      { handle_device }
		, m_handle_render_pass { handle_render_pass }
		, m_obj_render_context { obj_render_context }
	{
		create_descriptor_set_layout();

		std::vector<char> shaderCodeVert = TMP_VUlkanUtils::read_file_binary(vert_path);
		std::vector<char> shaderCodeFrag = TMP_VUlkanUtils::read_file_binary(frag_path);

		VkShaderModule shaderModuleVert = create_shader_module(handle_device, shaderCodeVert.data(), shaderCodeVert.size());
		VkShaderModule shaderModuleFrag = create_shader_module(handle_device, shaderCodeFrag.data(), shaderCodeFrag.size());

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
		rasterizer.cullMode = face_culling_mode;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

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

		// push constants
		VkPushConstantRange range = { VK_SHADER_STAGE_VERTEX_BIT };
		range.offset = 0;
		range.size = sizeof(DataUniformModel);

		// pipeline assembly
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_handle_descriptor_set_layout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		if (vkCreatePipelineLayout(m_handle_device, &pipelineLayoutInfo, NULL, &m_handle_pipeline_layout) != VK_SUCCESS)
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

		pipelineInfo.layout = m_handle_pipeline_layout;

		pipelineInfo.renderPass = m_handle_render_pass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1;              // Optional

		if (vkCreateGraphicsPipelines(m_handle_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &m_handle) != VK_SUCCESS)
			CC_LOG(ERROR, "failed to create graphics pipeline");

		// cleanup
		vkDestroyShaderModule(m_handle_device, shaderModuleFrag, NULL);
		vkDestroyShaderModule(m_handle_device, shaderModuleVert, NULL);

		create_uniform_buffers();
		create_texture_samplers();
		create_descriptor_sets();
	}

	Pipeline_FX::~Pipeline_FX() {
		vkDestroyPipelineLayout(m_handle_device, m_handle_pipeline_layout, NULL);
		vkDestroyPipeline(m_handle_device, m_handle, NULL);
		vkDestroyDescriptorSetLayout(m_handle_device, m_handle_descriptor_set_layout, NULL);
		vkDestroyDescriptorPool(m_handle_device, m_descriptor_pool, NULL);
		for (auto& handle : m_uniform_buffers)
			vkDestroyBuffer(m_handle_device, handle, NULL);
		for (auto& handle : m_uniform_buffers_memory)
			vkFreeMemory(m_handle_device, handle, NULL);

		vkDestroySampler(m_handle_device, m_texture_sampler, NULL);
	}

	void Pipeline_FX::update_uniform_buffer(DataUniformFrame& ubo, uint32_t current_frame) {
		memcpy(m_uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));
	}

	void Pipeline_FX::bind_descriptor_sets(VkCommandBuffer command_buffer, uint32_t image_index) {
		vkCmdBindDescriptorSets(
			command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_handle_pipeline_layout,
			0,
			1,
			&m_descriptor_sets[image_index],
			0, NULL
		);
	}

	void Pipeline_FX::create_descriptor_set_layout() {
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

		if (vkCreateDescriptorSetLayout(m_handle_device, &layoutInfo, NULL, &m_handle_descriptor_set_layout) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create descriptor set layout!");
		}
	}

	// TODO should this be automatically read from pipeline info?
	void Pipeline_FX::create_descriptor_sets() {
		uint8_t num_swapchain_images = m_obj_render_context->get_num_render_frames();

		// descriptor pool
		VkDescriptorPoolSize poolSize[] = {
		(VkDescriptorPoolSize) {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = num_swapchain_images
		},
		(VkDescriptorPoolSize) {
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = num_swapchain_images
		}
		};
		static const uint32_t poolSizeCount = sizeof(poolSize) / sizeof(VkDescriptorPoolSize);

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = poolSizeCount;
		poolInfo.pPoolSizes = poolSize;
		poolInfo.maxSets = num_swapchain_images;

		if (vkCreateDescriptorPool(m_handle_device, &poolInfo, NULL, &m_descriptor_pool) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create descriptor pool!");
		}

		std::vector<VkDescriptorSetLayout> layouts(num_swapchain_images);
		for (int i = 0; i < num_swapchain_images; ++i)
			layouts[i] = m_handle_descriptor_set_layout;

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptor_pool;
		allocInfo.descriptorSetCount = num_swapchain_images;
		allocInfo.pSetLayouts = layouts.data();

		m_descriptor_sets.resize(num_swapchain_images);

		if (vkAllocateDescriptorSets(m_handle_device, &allocInfo, m_descriptor_sets.data()) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < num_swapchain_images; i++) {
			VkDescriptorBufferInfo bufferInfo = { 0 };
			bufferInfo.buffer = m_uniform_buffers[i];
			bufferInfo.offset = 0;
			// TODO if UBO is application-specific, should we pass the size as parameter?
			bufferInfo.range = sizeof(DataUniformFrame);

			VkDescriptorImageInfo imageInfo = { 0 };
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = Drawcall::get_pipeline_image_view(); // TODO this is a consequence of using unified textures and samplers
			imageInfo.sampler = m_texture_sampler;

			VkWriteDescriptorSet descriptorWrites[] = {
				(VkWriteDescriptorSet) { // uniforms buffer
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_descriptor_sets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &bufferInfo
				},
				(VkWriteDescriptorSet) { // texture sampler
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_descriptor_sets[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &imageInfo
				}
			};
			static const uint32_t descriptorWritesCount = sizeof(descriptorWrites) / sizeof(VkWriteDescriptorSet);
			vkUpdateDescriptorSets(m_handle_device, descriptorWritesCount, descriptorWrites, 0, NULL);
		}
	}

	void Pipeline_FX::create_uniform_buffers() {
		VkDeviceSize bufferSize = sizeof(DataUniformFrame);
		uint8_t num_swapchain_images = m_obj_render_context->get_num_render_frames();

		m_uniform_buffers.resize(num_swapchain_images);
		m_uniform_buffers_memory.resize(num_swapchain_images);
		m_uniform_buffers_mapped.resize(num_swapchain_images);

		for (size_t i = 0; i < num_swapchain_images; ++i) {
			m_obj_render_context->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&m_uniform_buffers[i],
				&m_uniform_buffers_memory[i]
			);
			vkMapMemory(m_handle_device, m_uniform_buffers_memory[i], 0, bufferSize, 0, &m_uniform_buffers_mapped[i]);
		}
	}

	void Pipeline_FX::create_texture_samplers() {
		VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE; // ATM we are not requesting/enabling it in physical/logical device
		samplerInfo.maxAnisotropy = m_obj_render_context->get_physical_device_properties().limits.maxSamplerAnisotropy;
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

		if (vkCreateSampler(m_handle_device, &samplerInfo, NULL, &m_texture_sampler) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create texture sampler!");
		}
	}

	VkShaderModule Pipeline_FX::create_shader_module(VkDevice device, const char* code, const size_t codeSize) {
		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = codeSize;
		createInfo.pCode = (const uint32_t*)code;

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create shader module!");
		}

		return shaderModule;
	}
}