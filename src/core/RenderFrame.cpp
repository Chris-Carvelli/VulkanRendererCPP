#include "RenderFrame.hpp"

#include <VulkanUtils.h>
#include <core/RenderContext.hpp>
#include <core/Pipeline.hpp>
#include <core/DrawCall.hpp>

// probably not needed here, we just need access to the drawcall
#include <core/RenderContext.hpp>

// TMP test imgui
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <utils/DearImGui.hpp>

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
		DataUniformFrame ubo,
		const std::vector<Drawcall::DrawcallData>& drawcalls,
		const std::vector<Drawcall::DebugDrawcallData>& debug_drawcalls
	) {
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

		vkResetFences(m_device, 1, &m_fence_in_flight);

		vkResetCommandBuffer(m_command_buffer, 0);
		
		// cmd buffer =========================================================
		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = 0;               // Optional
		beginInfo.pInheritanceInfo = NULL; // Optional

		if (vkBeginCommandBuffer(m_command_buffer, &beginInfo) != VK_SUCCESS) {
			CC_LOG(ERROR, "failed to begin recording command buffer!");
		}

		// viewport and scissor ===============================================
		// these could be metadata of shaders/pipelines/renderpasses
		// examples of only some calls setting special ones
		// - minimap
		// - fake windows
		// deal with then when we'll need it
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
		// ====================================================================
		
		// renderpass =========================================================
		
		// ====================================================================

		// pipeline ===========================================================
		
		// ====================================================================

		// draw calls =========================================================
		vkc::RenderPass* obj_curr_render_pass = nullptr;
		vkc::Pipeline* obj_curr_pipeline = nullptr;
		VkRenderPassBeginInfo begin_info;

		for(const auto& drawcall : drawcalls)
		{
			if (drawcall.obj_render_pass != obj_curr_render_pass)
			{
				obj_curr_render_pass = drawcall.obj_render_pass;
				begin_info = obj_curr_render_pass->get_being_info(frame_index);
				vkCmdBeginRenderPass(
					m_command_buffer,
					&begin_info,
					VK_SUBPASS_CONTENTS_INLINE
				);
			}

			if (drawcall.obj_pipeline != obj_curr_pipeline)
			{
				obj_curr_pipeline = drawcall.obj_pipeline;
				vkCmdBindPipeline(
					m_command_buffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					obj_curr_pipeline->get_handle()
				);
				obj_curr_pipeline->bind_descriptor_sets(
					m_command_buffer,
					frame_index);
				obj_curr_pipeline->update_uniform_buffer(&ubo, frame_index);
				obj_curr_pipeline->update_uniform_buffer_material(drawcall.data_uniform_material, frame_index);
			}

			Drawcall::ModelDataGPU model_data_gpu = Drawcall::get_model_data(drawcall.idx_data_attributes);
			VkBuffer vertexBuffers[] = { model_data_gpu.vertex_buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdPushConstants(
				m_command_buffer,
				obj_curr_pipeline->get_layout(),
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				drawcall.data_uniform_model_size,
				drawcall.data_uniform_model
			);

			vkCmdBindVertexBuffers(m_command_buffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_command_buffer, model_data_gpu.index_buffer, 0, VK_INDEX_TYPE_UINT32);


			vkc::Instance::TMP_get_singleton_instance()->add_buffer_util_label(m_command_buffer, "424242 HELLO WORLD");
			vkCmdDrawIndexed(m_command_buffer, model_data_gpu.indices_count, 1, 0, 0, 0);
		}
		// ====================================================================

		// debug draw call ====================================================
		// TMP only one renderpass, debug drawcall hardcoded at idx 1
		auto obj_debug_pipeline = obj_curr_render_pass->get_debug_pipeline_ptr(0);
		vkCmdBindPipeline(
			m_command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			obj_debug_pipeline->get_handle()
		);
		obj_debug_pipeline->bind_descriptor_sets(
			m_command_buffer,
			frame_index
		);

		glm::mat4 viewProj = ubo.proj * ubo.view;
		obj_debug_pipeline->update_uniform_buffer(&viewProj, frame_index);

		// TODO IMMEDIATE bind pipeline and execute draw calls
		for (const auto& drawcall : debug_drawcalls)
		{
			vkCmdPushConstants(
				m_command_buffer,
				obj_debug_pipeline->get_layout(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(drawcall.data_uniform_model),
				&drawcall.data_uniform_model
			);


			Drawcall::ModelDataGPU model_data_gpu = Drawcall::get_model_data(drawcall.idx_data_attributes);
			VkBuffer vertexBuffers[] = { model_data_gpu.vertex_buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_command_buffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_command_buffer, model_data_gpu.index_buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(m_command_buffer, model_data_gpu.indices_count, 1, 0, 0, 0);
		}
		// ====================================================================
		
		// TMP test imgui

		// TODO check if RenderDrawData's third parameter (VkPipeline, default to nullptr) is needed
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_command_buffer);

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