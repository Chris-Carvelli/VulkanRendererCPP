#include "Pipeline.hpp"

#include <VulkanUtils.h>
#include <core/RenderContext.hpp>
#include <core/DrawCall.hpp>

namespace vkc {
	Pipeline::Pipeline(
		VkDevice handle_device,
		vkc::RenderContext* obj_render_context,
		vkc::RenderPass* obj_render_pass,
		PipelineConfig* config
	)
		: m_handle_device      { handle_device }
		, m_obj_render_pass { obj_render_pass }
		, m_obj_render_context { obj_render_context }
		, m_config { config }
	{
		create_descriptor_set_layout();

		std::vector<char> shaderCodeVert = TMP_VUlkanUtils::read_file_binary(config->vert_path);
		std::vector<char> shaderCodeFrag = TMP_VUlkanUtils::read_file_binary(config->frag_path);

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
		vertexInputInfo.vertexBindingDescriptionCount = config->vertex_binding_descriptors_count;
		vertexInputInfo.pVertexBindingDescriptions = config->vertex_binding_descriptors;
		vertexInputInfo.vertexAttributeDescriptionCount = config->vertex_attribute_descriptors_count;
		vertexInputInfo.pVertexAttributeDescriptions = config->vertex_attribute_descriptors;

		// input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssembly.topology = config->topology;
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
		rasterizer.cullMode = config->face_culling_mode;
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
		depthStencil.depthCompareOp = m_config->compare_op;
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

		// push constants
		VkPushConstantRange push_constant_range;
		uint32_t push_constant_range_count = 0;
		if (config->size_push_constant_model > 0)
		{
			push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			push_constant_range.offset     = 0;
			push_constant_range.size       = config->size_push_constant_model;
			push_constant_range_count = 1;
		}

		// pipeline assembly
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_handle_descriptor_set_layout;
		pipelineLayoutInfo.pushConstantRangeCount = push_constant_range_count;
		pipelineLayoutInfo.pPushConstantRanges = &push_constant_range;

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

		pipelineInfo.renderPass = m_obj_render_pass->get_handle();
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

	Pipeline::~Pipeline() {
		if (m_handle == VK_NULL_HANDLE)
			return;
		vkDestroyPipelineLayout(m_handle_device, m_handle_pipeline_layout, NULL);
		vkDestroyPipeline(m_handle_device, m_handle, NULL);
		vkDestroyDescriptorSetLayout(m_handle_device, m_handle_descriptor_set_layout, NULL);
		vkDestroyDescriptorPool(m_handle_device, m_descriptor_pool, NULL);
		for (auto& handle : m_uniform_buffers)
			vkDestroyBuffer(m_handle_device, handle, NULL);
		for (auto& handle : m_uniform_buffers_memory)
			vkFreeMemory(m_handle_device, handle, NULL);
		for (auto& handle : m_uniform_buffers_material)
			vkDestroyBuffer(m_handle_device, handle, NULL);
		for (auto& handle : m_uniform_buffers_memory_material)
			vkFreeMemory(m_handle_device, handle, NULL);

		for(auto &sampler : m_texture_samplers)
			vkDestroySampler(m_handle_device, sampler, NULL);

		// FIXME cleanup config (see below)
		// pipeline either acquires ALL config arrays, or none
		// a good idea could be to follow Box2D pattern and just memcpy
		// config data, so we can take care of all our stuff
		delete m_config->texture_image_views;
		delete m_config;
	}

	void Pipeline::update_uniform_buffer(void* ubo, uint32_t current_frame) {
		memcpy(m_uniform_buffers_mapped[current_frame], ubo, m_config->size_uniform_data_frame);
	}

	void Pipeline::update_uniform_buffer_material(void* ubo, uint32_t current_frame) {
		if (ubo == nullptr)
			return;

		memcpy(m_uniform_buffers_mapped_material[current_frame], ubo, m_config->size_uniform_data_material);
	}


	void Pipeline::update_material_textures(std::vector<VkImageView> image_views, uint32_t current_frame) {
		for (int j = 0; j < m_config->texture_image_views_count; ++j)
		{
			VkDescriptorImageInfo imageInfo;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = image_views[j];
			imageInfo.sampler = m_texture_samplers[j];

			VkWriteDescriptorSet descriptor_writes = (VkWriteDescriptorSet){ // texture sampler
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_descriptor_sets[current_frame],
				.dstBinding = m_first_texture_binding_slot + j,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo
			};

			vkUpdateDescriptorSets(m_handle_device, 1, &descriptor_writes, 0, NULL);
		}
	}

	void Pipeline::bind_descriptor_sets(VkCommandBuffer command_buffer, uint32_t image_index) {
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

	void Pipeline::create_descriptor_set_layout() {
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		uint32_t idx_binding = 0;

		VkDescriptorSetLayoutBinding uboLayoutBinding_frame = { 0 };
		uboLayoutBinding_frame.binding = idx_binding++;
		uboLayoutBinding_frame.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding_frame.descriptorCount = 1;
		uboLayoutBinding_frame.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings.push_back(uboLayoutBinding_frame);

		if (m_config->size_uniform_data_material > 0)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding_material = { 0 };
			uboLayoutBinding_material.binding = idx_binding++;
			uboLayoutBinding_material.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding_material.descriptorCount = 1;
			uboLayoutBinding_material.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings.push_back(uboLayoutBinding_material);
		}

		//if (m_config->texture_image_views_count > 0)
		for(int i = 0; i < m_config->texture_image_views_count; ++i)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding = { 0 };
			samplerLayoutBinding.binding = idx_binding++;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings.push_back(samplerLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(m_handle_device, &layoutInfo, NULL, &m_handle_descriptor_set_layout) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create descriptor set layout!");
		}
	}

	void Pipeline::create_descriptor_sets() {
		uint8_t num_swapchain_images = m_obj_render_context->get_num_render_frames();

		// descriptor pool
		std::vector<VkDescriptorPoolSize> poolSize = {
			(VkDescriptorPoolSize) {  // frame uniform - always present (for now)
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = num_swapchain_images
			}
		};

		// material - onlt if material uniform size > 0
		if (m_config->size_uniform_data_material > 0) {
			poolSize.push_back((VkDescriptorPoolSize) {
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = num_swapchain_images
			});
		}

		// texture - only if num textures > 0
		//if (m_config->texture_image_views_count > 0)
		m_first_texture_binding_slot = poolSize.size();
		for(int i = 0; i < m_config->texture_image_views_count; ++i)
		{
			poolSize.push_back((VkDescriptorPoolSize) {
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = num_swapchain_images
			});
		}

		uint32_t poolSizeCount = poolSize.size();

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = poolSizeCount;
		poolInfo.pPoolSizes = poolSize.data();
		poolInfo.maxSets = num_swapchain_images;

		if (vkCreateDescriptorPool(m_handle_device, &poolInfo, NULL, &m_descriptor_pool) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create descriptor pool!");
		}

		std::vector<VkDescriptorSetLayout> layouts(num_swapchain_images);
		for (int i = 0; i < num_swapchain_images; ++i)
			layouts[i] = m_handle_descriptor_set_layout;

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = m_descriptor_pool;
		allocInfo.descriptorSetCount = num_swapchain_images;
		allocInfo.pSetLayouts = layouts.data();

		m_descriptor_sets.resize(num_swapchain_images);

		CC_VK_CHECK(vkAllocateDescriptorSets(m_handle_device, &allocInfo, m_descriptor_sets.data()));


		for (size_t i = 0; i < num_swapchain_images; ++i) {
			std::vector< VkWriteDescriptorSet> descriptor_writes(poolSizeCount);

			VkDescriptorBufferInfo bufferInfo_frame = { 0 };
			bufferInfo_frame.buffer = m_uniform_buffers[i];
			bufferInfo_frame.offset = 0;
			bufferInfo_frame.range = m_config->size_uniform_data_frame;

			descriptor_writes[0] = (VkWriteDescriptorSet){ // uniforms buffer - frame
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_descriptor_sets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &bufferInfo_frame
			};

			if (m_config->size_uniform_data_material > 0) {
				VkDescriptorBufferInfo bufferInfo_material = { 0 };
				bufferInfo_material.buffer = m_uniform_buffers_material[i];
				bufferInfo_material.offset = 0;
				bufferInfo_material.range = m_config->size_uniform_data_material;
				descriptor_writes[1] = (VkWriteDescriptorSet){ // uniforms buffer - material
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = m_descriptor_sets[i],
						.dstBinding = 1,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.pBufferInfo = &bufferInfo_material
				};
			}

			std::vector<VkDescriptorImageInfo> imageInfo = std::vector<VkDescriptorImageInfo>(m_config->texture_image_views_count);


			for (int j = 0; j < m_config->texture_image_views_count; ++j)
			{
				imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo[j].imageView = m_config->texture_image_views[j];
				imageInfo[j].sampler = m_texture_samplers[j];

				uint32_t idx_binding = m_first_texture_binding_slot + j;
				descriptor_writes[idx_binding] = (VkWriteDescriptorSet){ // texture sampler
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_descriptor_sets[i],
					.dstBinding = idx_binding,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &imageInfo[j]
				};
			}

			vkUpdateDescriptorSets(m_handle_device, poolSizeCount, descriptor_writes.data(), 0, NULL);
		}
	}


	void Pipeline::create_uniform_buffers() {
		// frame data
		{
			VkDeviceSize bufferSize = m_config->size_uniform_data_frame;
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

		// material data
		if(m_config->size_uniform_data_material > 0)
		{
			VkDeviceSize bufferSize = m_config->size_uniform_data_material;
			uint8_t num_swapchain_images = m_obj_render_context->get_num_render_frames();

			m_uniform_buffers_material.resize(num_swapchain_images);
			m_uniform_buffers_memory_material.resize(num_swapchain_images);
			m_uniform_buffers_mapped_material.resize(num_swapchain_images);

			for (size_t i = 0; i < num_swapchain_images; ++i) {
				m_obj_render_context->createBuffer(
					bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&m_uniform_buffers_material[i],
					&m_uniform_buffers_memory_material[i]
				);
				vkMapMemory(m_handle_device, m_uniform_buffers_memory_material[i], 0, bufferSize, 0, &m_uniform_buffers_mapped_material[i]);
			}
		}
	}

	void Pipeline::create_texture_samplers() {
		m_texture_samplers.resize(m_config->texture_image_views_count);


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

		for(int i = 0; i < m_config->texture_image_views_count; ++i)
			CC_VK_CHECK(vkCreateSampler(m_handle_device, &samplerInfo, NULL, &m_texture_samplers[i]));
	}

	VkShaderModule Pipeline::create_shader_module(VkDevice device, const char* code, const size_t codeSize) {
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