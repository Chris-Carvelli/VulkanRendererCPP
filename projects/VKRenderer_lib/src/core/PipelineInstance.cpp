#include "PipelineInstance.hpp"

#include <VulkanUtils.h>
#include <core/RenderContext.hpp>

namespace vkc {
	PipelineInstance::PipelineInstance(
		VkDevice handle_device,
		vkc::RenderContext* obj_render_context,
		vkc::RenderPass* obj_render_pass,
		vkc::Pipeline* obj_pipeline,
		std::vector<VkImageView> image_views
	)
		: m_handle_device      { handle_device }
		, m_obj_render_context { obj_render_context }
		, m_obj_render_pass    { obj_render_pass }
		, m_obj_pipeline       { obj_pipeline }
		, m_image_views        { image_views }
	{
		m_config = obj_pipeline->get_obj_config();

		CC_ASSERT(
			image_views.size() == m_config->texture_slots_count,
			"[PipelineInstance] material config expecting %d textures, %d provided",
			image_views.size(),
			m_config->texture_slots_count
		);

		create_texture_samplers();
		create_material_uniform_buffers();

		create_descriptor_sets();
		update_descriptor_sets();
	}

	PipelineInstance::~PipelineInstance() {
		destroy_descriptor_sets();

		for (auto& handle : m_uniform_buffers_material)
			vkDestroyBuffer(m_handle_device, handle, NULL);
		for (auto& handle : m_uniform_buffers_memory_material)
			vkFreeMemory(m_handle_device, handle, NULL);

		for(auto &sampler : m_texture_samplers)
			vkDestroySampler(m_handle_device, sampler, NULL);
	}

	void PipelineInstance::create_descriptor_sets() {
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
		for(int i = 0; i < m_config->texture_slots_count; ++i)
		{
			poolSize.push_back((VkDescriptorPoolSize) {
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = num_swapchain_images
			});
		}

		m_pool_size_count = poolSize.size();

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = m_pool_size_count;
		poolInfo.pPoolSizes = poolSize.data();
		poolInfo.maxSets = num_swapchain_images;

		if (vkCreateDescriptorPool(m_handle_device, &poolInfo, NULL, &m_descriptor_pool) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to create descriptor pool!");
		}

		std::vector<VkDescriptorSetLayout> layouts(num_swapchain_images);
		for (int i = 0; i < num_swapchain_images; ++i)
			layouts[i] = m_obj_pipeline->get_handle_descriptor_set_layout();

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = m_descriptor_pool;
		allocInfo.descriptorSetCount = num_swapchain_images;
		allocInfo.pSetLayouts = layouts.data();

		m_descriptor_sets.resize(num_swapchain_images);

		CC_VK_CHECK(vkAllocateDescriptorSets(m_handle_device, &allocInfo, m_descriptor_sets.data()));
	}

	void PipelineInstance::update_descriptor_sets() {
		uint8_t num_swapchain_images = m_obj_render_context->get_num_render_frames();

		for (size_t i = 0; i < num_swapchain_images; ++i) {
			std::vector< VkWriteDescriptorSet> descriptor_writes(m_pool_size_count);

			VkDescriptorBufferInfo bufferInfo_frame = { 0 };
			bufferInfo_frame.buffer = m_obj_pipeline->get_handle_uniform_buffer(i);
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

			std::vector<VkDescriptorImageInfo> imageInfo = std::vector<VkDescriptorImageInfo>(m_config->texture_slots_count);
			for (int j = 0; j < m_config->texture_slots_count; ++j)
			{
				imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo[j].imageView = m_image_views[j];
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

			vkUpdateDescriptorSets(m_handle_device, m_pool_size_count, descriptor_writes.data(), 0, NULL);
		}
	}
	void PipelineInstance::destroy_descriptor_sets() {
		vkDestroyDescriptorPool(m_handle_device, m_descriptor_pool, VK_NULL_HANDLE);
	}

	void PipelineInstance::create_material_uniform_buffers()
	{
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

	void PipelineInstance::create_texture_samplers() {
		m_texture_samplers.resize(m_config->texture_slots_count);

		VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		// mipmapping
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE; // no clamping

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE; // ATM we are not requesting/enabling it in physical/logical device
		//samplerInfo.maxAnisotropy = m_obj_render_context->get_physical_device_properties().limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		// percentage-closer filtering (shadow mapping)
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		for(int i = 0; i < m_config->texture_slots_count; ++i)
			CC_VK_CHECK(vkCreateSampler(m_handle_device, &samplerInfo, NULL, &m_texture_samplers[i]));
	}

	void PipelineInstance::bind_descriptor_sets(VkCommandBuffer command_buffer, uint32_t image_index) {
		vkCmdBindDescriptorSets(
			command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_obj_pipeline->get_handle_layout(),
			0,
			1,
			&m_descriptor_sets[image_index],
			0, NULL
		);
	}

	void PipelineInstance::update_uniform_buffer_material(void* ubo, uint32_t current_frame) {
		if (ubo == nullptr)
			return;

		memcpy(m_uniform_buffers_mapped_material[current_frame], ubo, m_config->size_uniform_data_material);
	}
}