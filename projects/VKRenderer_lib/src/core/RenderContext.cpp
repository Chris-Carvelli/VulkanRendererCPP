#include "RenderContext.hpp"

#include <VulkanUtils.h>
#include <window/Window.hpp>
#include <core/PhysicalDevice.hpp>
#include <core/DrawCall.hpp>

#include <core/VertexData.h>

#include <AssetManager.hpp>

namespace vkc {

    // TODO fix these long constructor? (dependency injection good I guess, not sure about this)
    RenderContext::RenderContext(
        const PhysicalDevice *physical_device,
        VkDevice device,
        VkSurfaceKHR surface,
        Window* window
    ) {
        assert(window != nullptr);

        m_window = window;
        m_obj_physical_device = physical_device;
        m_device = device;

        m_swapchain = std::make_unique<Swapchain>(physical_device->get_handle(), device, surface, window->get_current_extent());

        const uint32_t num_frames_in_flight = 3;

        // command pool
        QueueFamilyIndices queueFamilyIndices = physical_device->find_queue_families();

        VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

        if (vkCreateCommandPool(device, &poolInfo, NULL, &m_command_pool) != VK_SUCCESS)
            CC_LOG(ERROR, "failed to create command pool");

        // pre-allocate command buffer
        std::vector<VkCommandBuffer> command_buffers;
        command_buffers.resize(num_frames_in_flight);

        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_command_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = num_frames_in_flight;

        if (vkAllocateCommandBuffers(device, &allocInfo, command_buffers.data()) != VK_SUCCESS) {
            CC_LOG(ERROR, "failed to allocate command buffers!");
        }


        // queues (needed to initialize frames, so do it before)
        QueueFamilyIndices indices = physical_device->find_queue_families();
        vkGetDeviceQueue(device, indices.graphicsFamily, 0, &m_queue_graphic);
        vkGetDeviceQueue(device, indices.presentFamily, 0, &m_queue_present);

        // TODO move heavy vulkan work into `prepare` or `init` methods
        m_frames.resize(num_frames_in_flight);
        for (int i = 0; i < num_frames_in_flight; ++i)
            m_frames[i] = std::make_unique<RenderFrame>(
                device,
                this,
                command_buffers[i]
            );

        // create defulta renderpass
        add_renderpass();
    }

    RenderContext::~RenderContext() {
        Drawcall::destroy_resources(m_device);
        vkDestroyCommandPool(m_device, m_command_pool, NULL);
    }

    const VkPhysicalDeviceProperties& RenderContext::get_physical_device_properties() const {
        return m_obj_physical_device->get_physical_device_properties();
    }

    void RenderContext::render_begin() {
        // return;
        
        // TODO FIXME find an appropriate place for drawing defualt gizmos
        // center
        Drawcall::add_debug_cube(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.1f, 0.1f, 0.1f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );

        // right
        Drawcall::add_debug_cube(
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.1f, 0.1f, 0.1f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        // up
        Drawcall::add_debug_cube(
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.1f, 0.1f, 0.1f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // forward
        Drawcall::add_debug_cube(
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.1f, 0.1f, 0.1f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        Drawcall::add_debug_ray(
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            1.0f,
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        Drawcall::add_debug_ray(
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            1.0f,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        Drawcall::add_debug_ray(
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            1.0f,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
    }

    void RenderContext::render_finalize() {
        m_frames[m_active_frame_index]->render(
            m_swapchain->get_handle(),
            m_queue_graphic,
            m_queue_present,
            m_active_frame_index,
            m_swapchain->get_extent(),
            m_ubo,
            Drawcall::get_drawcalls(),
            Drawcall::get_debug_drawcalls()
        );

        m_active_frame_index = (m_active_frame_index + 1) % get_num_render_frames();

        //clear drawcalls
        Drawcall::clear_drawcalls();
        Drawcall::clear_debug_drawcalls();
    }

    void RenderContext::update_mesh_vertex_data(uint32_t mesh_index, void* vertex_data, uint32_t vertex_data_size) {
        vkc::Drawcall::updateModelVertexBuffer(mesh_index, vertex_data, vertex_data_size, m_device, this);
    }

    void RenderContext::recreate_swapchain() {
        assert(m_window != nullptr);
        m_swapchain->recreate(m_window->get_current_extent());

        for (auto& render_pass : m_render_passes)
            // TODO FIXME this is actually a renderpass method
            //  (recreates framebuffers and assciated image attachments)
            render_pass->handle_swapchain_recreation();
    }

    VkCommandBuffer RenderContext::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_command_pool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);


        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void RenderContext::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        CC_VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        CC_VK_CHECK(vkQueueSubmit(m_queue_graphic, 1, &submitInfo, VK_NULL_HANDLE));
        CC_VK_CHECK(vkQueueWaitIdle(m_queue_graphic));

        vkFreeCommandBuffers(m_device, m_command_pool, 1, &commandBuffer);
    }

    void RenderContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBuffer cmdbuf = beginSingleTimeCommands();
        VkBufferCopy copyRegion = { 0 };
        copyRegion.size = size;
        vkCmdCopyBuffer(cmdbuf, src, dst, 1, &copyRegion);
        endSingleTimeCommands(cmdbuf);
    }

    void RenderContext::copy_buffer_to_image(VkBuffer buffer, VkImage image, const vkc::Assets::TextureData& data) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy* regions = (VkBufferImageCopy*)calloc(data.mipmaps, sizeof(VkBufferImageCopy));

        uint32_t block_w, block_h;
        size_t bytes_per_texel;


        VkFormat f = (VkFormat)data.format;
        get_block_sizes(f, block_w, block_h, bytes_per_texel);
        uint32_t mip_level_w = data.width;
        uint32_t mip_level_h = data.height;
        VkDeviceSize buffer_offset = 0;


        if (data.mipmaps > 1)
            auto a = 0;

        for(int i = 0; i < data.mipmaps; ++i)
        {
            //VkBufferImageCopy& region = regions[i];
            regions[i].bufferOffset = buffer_offset;
            regions[i].bufferRowLength = 0;
            regions[i].bufferImageHeight = 0;
            regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[i].imageSubresource.mipLevel = i;
            regions[i].imageSubresource.baseArrayLayer = 0;
            regions[i].imageSubresource.layerCount = data.viewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
            regions[i].imageOffset = (VkOffset3D){
                .x = 0,
                .y = 0,
                .z = 0
            };
            regions[i].imageExtent = (VkExtent3D){
                .width  = mip_level_w,
                .height = mip_level_h,
                .depth = 1
            };


            VkDeviceSize buffer_offset_add;
            if (bytes_per_texel >= 16)
                buffer_offset_add = mip_level_w * mip_level_h * bytes_per_texel / 16;
            else if (bytes_per_texel < 16)
                buffer_offset_add = mip_level_w * mip_level_h / (16 / bytes_per_texel);

            buffer_offset += glm::max(buffer_offset_add, bytes_per_texel);

            // round
            //CC_ASSERT(buffer_offset % block_size == 0, "ivalid block alignment");
            /*if (buffer_offset % texel_size != 0)
                buffer_offset += texel_size - buffer_offset % texel_size;*/
            mip_level_w /= 2;
            mip_level_h /= 2;
        }

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            data.mipmaps,
            regions
        );

        free(regions);

        endSingleTimeCommands(commandBuffer);
    }

    void RenderContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory) {
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device, &bufferInfo, NULL, pBuffer) != VK_SUCCESS) {
            CC_LOG(ERROR, "failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, *pBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_obj_physical_device->find_memory_type(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device, &allocInfo, NULL, pBufferMemory) != VK_SUCCESS) {
            CC_LOG(ERROR, "failed to allocate buffer memory!");
        }

        vkBindBufferMemory(m_device, *pBuffer, *pBufferMemory, 0);
    }

    void RenderContext::transition_image_layout(VkImage image, uint32_t layers, uint32_t mip_levels, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mip_levels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layers;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            CC_LOG(ERROR, "unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, NULL,
            0, NULL,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void RenderContext::create_image(uint32_t width, uint32_t height, uint32_t layers, uint32_t mip_levels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* pImage, VkDeviceMemory* pImageMemory) {
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mip_levels;
        imageInfo.arrayLayers = layers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (layers > 1) {
            // TMP FIXME handle cubemap
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        if (vkCreateImage(m_device, &imageInfo, NULL, pImage) != VK_SUCCESS) {
            CC_LOG(ERROR, "failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, *pImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_obj_physical_device->find_memory_type(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device, &allocInfo, NULL, pImageMemory) != VK_SUCCESS) {
            CC_LOG(ERROR, "failed to allocate image memory!");
        }

        vkBindImageMemory(m_device, *pImage, *pImageMemory, 0);
    }

    VkImageView RenderContext::create_imge_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t mip_levels) {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = image;
        createInfo.viewType = viewType;
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = aspectFlags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = mip_levels;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = viewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

        VkImageView imageView;
        if (vkCreateImageView(m_device, &createInfo, NULL, &imageView) != VK_SUCCESS) {
            CC_LOG(ERROR,"failed to create image views");
        }

        return imageView;
    }
}