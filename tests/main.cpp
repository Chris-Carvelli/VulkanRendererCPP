#include <VKRenderer.hpp>


class TestRenderer : public VKRenderer {
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;

    void prepare() {
        // TODO load assets
        // TODO prepare pipelines
        
        // build command buffers
        VkCommandBuffer commandBuffer = get_current_command_buffer();
        VkFramebuffer swapchain_framebuffer = get_current_swapchain_framebuffer();
        VkRenderPass render_pass = get_current_renderpass();
        VkExtent2D swapchain_extent = get_swapchain_extent();

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = 0;               // Optional
        beginInfo.pInheritanceInfo = NULL; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            CC_LOG(ERROR, "failed to begin recording command buffer!");
        }

        // one per attachment, in attachment order
        const VkClearValue clearValues[] = {
            (VkClearValue) { (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 1.0f }} },
            (VkClearValue) { (VkClearDepthStencilValue){{ 1.0f, 0 }} }
        };
        const int clearValuesCount = sizeof(clearValues) / sizeof(VkClearValue);

        VkRenderPassBeginInfo renderPassInfo = { };
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = render_pass;
        renderPassInfo.framebuffer = swapchain_framebuffer;
        renderPassInfo.renderArea.offset = (VkOffset2D){
            .x = 0,
            .y = 0
        };
        renderPassInfo.renderArea.extent = swapchain_extent;
        renderPassInfo.clearValueCount = clearValuesCount;
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        VkViewport viewport = { };
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = swapchain_extent.width;
        viewport.height = swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = { };
        scissor.offset = (VkOffset2D){
            .x = 0,
            .y = 0
        };
        scissor.extent = swapchain_extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
            0, 1, &descriptorSets[currentFrame],
            0, NULL
        );
        vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            CC_LOG(ERROR, "failed to record command buffer");
    }
    void render() {

        //// application ============
        //updateUniformBuffer(currentFrame);
        //recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        //// application end ========
    }
};

int main() {
    TestRenderer app;

    app.run();

    return 0;
}