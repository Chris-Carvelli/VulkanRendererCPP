#include "RenderFrame.hpp"

#include <VulkanUtils.h>
#include <core/RenderContext.hpp>
#include <core/Pipeline.hpp>
#include <core/DrawCall.hpp>

// probably not needed her,e we just need access to the drawcall
#include <core/RenderContext.hpp>

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
	}

	RenderFrame::~RenderFrame() {
		vkDestroyFence(m_device, m_fence_in_flight, NULL);
		vkDestroySemaphore(m_device, m_semaphore_render_finished, NULL);
		vkDestroySemaphore(m_device, m_semaphore_image_available, NULL);
	}

	void RenderFrame::render(
		VkSwapchainKHR swapchain,
		VkQueue queue_render,
		VkQueue queue_present,
		uint32_t frame_index,
		VkExtent2D swapchain_extent,
		VkRenderPassBeginInfo render_pass_info,
		vkc::Pipeline* obj_pipeline,
		DataUniformFrame ubo,
		const std::vector<Drawcall::DrawcallData>& drawcalls
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

		// this will be executed reading the uniform data form a queue of DrawCalls
		obj_pipeline->update_uniform_buffer(ubo, frame_index);

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
		vkCmdBeginRenderPass(m_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		// ====================================================================

		// pipeline ===========================================================
		vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj_pipeline->get_handle());
		obj_pipeline->bind_descriptor_sets(m_command_buffer, frame_index);
		// ====================================================================

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

		// command buffer =====================================================
		int c = 0;
		for(const auto& drawcall : drawcalls)
		{
			++c;
			Drawcall::ModelDataGPU model_data_gpu = Drawcall::get_model_data(drawcall.idx_model_data);
			VkBuffer vertexBuffers[] = { model_data_gpu.vertex_buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdPushConstants(
				m_command_buffer,
				obj_pipeline->get_layout(),
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(drawcall.model_data),
				&drawcall.model_data
			);
			vkCmdBindVertexBuffers(m_command_buffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_command_buffer, model_data_gpu.index_buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(m_command_buffer, model_data_gpu.indices_count, 1, 0, 0, 0);
		}
		// ====================================================================


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
}