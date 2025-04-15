#include "RenderPass.hpp"

#include <VulkanUtils.h>
#include <core/RenderContext.hpp>
#include <core/DrawCall.hpp>

#include <core/VertexData.h>

namespace vkc {
	RenderPass::RenderPass(VkDevice device, RenderContext* obj_render_context)
		: m_handle_device{ device }
		, m_obj_render_context{ obj_render_context }
	{
		VkAttachmentDescription attachmentDescriptions[] = {
			(VkAttachmentDescription) { // color attachment
				.format = obj_render_context->get_swapchain_image_format(),
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

		if (vkCreateRenderPass(device, &renderPassInfo, NULL, &m_handle) != VK_SUCCESS)
			CC_LOG(ERROR, "failed to create render pass");

		create_depth_resources();
		create_framebuffers();
		create_pipelines();
	}

	RenderPass::~RenderPass() {
		vkDestroyImageView(m_handle_device, m_depth_image_view, NULL);
		vkDestroyImage(m_handle_device, m_depth_image, NULL);
		vkFreeMemory(m_handle_device, m_depth_image_memory, NULL);

		for (auto& handle : m_handle_framebuffers)
			vkDestroyFramebuffer(m_handle_device, handle, NULL);

		vkDestroyRenderPass(m_handle_device, m_handle, NULL);
	}

	void RenderPass::handle_swapchain_recreation() {
		vkDeviceWaitIdle(m_handle_device);

		handle_swapchain_destruction();

		create_depth_resources();
		create_framebuffers();
	}

	VkRenderPassBeginInfo RenderPass::get_being_info(uint8_t frame_index) const {
		VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassInfo.renderPass = m_handle;
		renderPassInfo.framebuffer = m_handle_framebuffers[frame_index]; // FIXME one per in-flight-frame
		renderPassInfo.renderArea.offset = (VkOffset2D){
			.x = 0,
			.y = 0
		};

		renderPassInfo.renderArea.extent = m_obj_render_context->get_swapchain_extent();
		renderPassInfo.clearValueCount = m_clear_values_count;
		renderPassInfo.pClearValues = m_clear_values;

		return renderPassInfo;
	}

	uint8_t RenderPass::get_pipelines_count() const {
		return m_pipelines.size();
	}

	VkPipeline RenderPass::get_pipeline_handle(uint8_t i) {
		assert(i >= 0 && i < m_pipelines.size());
		return m_pipelines[i]->get_handle();
	}

	vkc::Pipeline* RenderPass::get_pipeline_ptr(uint8_t i) {
		assert(i >= 0 && i < m_pipelines.size());
		return m_pipelines[i].get();
	}


	vkc::DebugPipeline* RenderPass::get_debug_pipeline_ptr(uint8_t i) {
		assert(i >= 0 && i < m_debug_pipelines.size());
		return m_debug_pipelines[i].get();
	}

	uint32_t RenderPass::add_pipeline(PipelineConfig* config) {
		uint32_t ret = m_pipelines.size();
		m_pipelines.push_back(std::make_unique<vkc::Pipeline>(
			m_handle_device,
			m_obj_render_context,
			this,
			config
		));

		return ret;
	}

	void RenderPass::create_framebuffers() {
		VkExtent2D extents = m_obj_render_context->get_swapchain_extent();
		int num_render_frames = m_obj_render_context->get_num_render_frames();
		VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass = m_handle;
		framebufferInfo.width = extents.width;
		framebufferInfo.height = extents.height;
		framebufferInfo.layers = 1;

		int i = 0;
		m_handle_framebuffers.resize(num_render_frames);
		for (VkImageView swapchain_image_view : m_obj_render_context->get_obj_swapchain()->get_image_views())
		{
			const int attachmentsCount = 2;
			VkImageView attachments[] = {
				swapchain_image_view,
				m_depth_image_view
			};

			framebufferInfo.attachmentCount = attachmentsCount;
			framebufferInfo.pAttachments = attachments;
			if (vkCreateFramebuffer(m_handle_device, &framebufferInfo, NULL, &m_handle_framebuffers[i]) != VK_SUCCESS)
				CC_LOG(ERROR, "failed to create framebuffer!");
			++i;
		}
	}

	void RenderPass::create_depth_resources() {
		VkExtent2D extent = m_obj_render_context->get_swapchain_extent();
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // TODO findDepthFormat()

		m_obj_render_context->create_image(
			extent.width,
			extent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&m_depth_image,
			&m_depth_image_memory
		);
		m_depth_image_view = m_obj_render_context->create_imge_view(
			m_depth_image,
			depthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT
		);
	}

	void RenderPass::create_pipelines() {
		m_debug_pipelines.resize(1);

		// debug volumes
		m_debug_pipelines[0] = std::make_unique<vkc::DebugPipeline>(
			m_handle_device,
			m_obj_render_context,
			this,
			new DebugPipelineConfig{
				.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST
			}
		);

		// // TODO
		// // 1. RenderPass must be created with custom framebuffers
		// // 2. render textures must be resized with swapchain recreation
		//// outlines
		//m_pipelines[1] = std::make_unique<vkc::Pipeline>(
		//	m_handle_device,
		//	m_obj_render_context,
		//	m_handle,
		//	new PipelineConfig{
		//		"res/shaders/fx_shader_fullscreen.vert.spv",
		//		"res/shaders/fx_shader.frag.spv",
		//		.size_uniform_data_frame = 0,
		//		.size_uniform_data_material = 0,
		//		.size_push_constant_model = 0,
		//		.vertex_binding_descriptors = vertexData_getBindingDescriptions(),
		//		.vertex_binding_descriptors_count = vertexData_getBindingDescriptionsCount(),
		//		// FIXME check `vertexDataFX_getAttributeDescriptions` definition
		//		.vertex_attribute_descriptors = vertexDataFX_getAttributeDescriptions(),
		//		.vertex_attribute_descriptors_count = 1,
		//		.texture_image_views = new VkImageView[] {
		//			m_colo
		//			m_depth_image_view
		//		},
		//		.texture_image_views_count = 2,
		//		.face_culling_mode = VK_CULL_MODE_FRONT_BIT
		//	}
		//);
	}

	void RenderPass::handle_swapchain_destruction() {
		vkDestroyImageView(m_handle_device, m_depth_image_view, NULL);
		vkDestroyImage(m_handle_device, m_depth_image, NULL);
		vkFreeMemory(m_handle_device, m_depth_image_memory, NULL);

		for (uint32_t i = 0; i < m_handle_framebuffers.size(); ++i)
			vkDestroyFramebuffer(m_handle_device, m_handle_framebuffers[i], NULL);
	}
}