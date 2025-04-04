#include "RenderContext.hpp"

#include <VulkanUtils.h>
#include <window/Window.hpp>
#include <core/PhysicalDevice.hpp>
#include <core/DrawCall.hpp>

#include <core/VertexData.h>

// TMP_Update includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// TMP_Assets includes
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include <map>
#include <filesystem>
#include <string.h>

// TMP_Update includes
#include <imgui.h>

namespace TMP_Assets {
    int num_mesh_assets;

    //int num_mesh_assets = sizeof(model_paths) / sizeof(model_paths[0]);

    std::map<uint32_t, MeshData> mesh_data;
    std::map<uint32_t, TextureData> texture_data;

    // only one texture for now
    TextureData& get_texture_data(uint32_t index) {
        return texture_data[index];
    }

    MeshData& get_mesh_data(uint32_t index) {
        return mesh_data[index];
    }

    MeshData load_mesh(const char* path, bool invert_x_y=false) {
        int offset_x = 0;
        int offset_y = invert_x_y ? 2 : 1;
        int offset_z = invert_x_y ? 1 : 2;
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
        for (const auto& shape : shapes)
            for (const auto& index : shape.mesh.indices) {
                ret.indices[i] = i;
                // flipped X and Y to match Kenney assets orientation
                // (prob. default blender)
                // this should probably be an import util + cooking anyway
                ret.vertices[i].position = glm::vec3(
                    attrib.vertices[3 * index.vertex_index + offset_x],
                    attrib.vertices[3 * index.vertex_index + offset_y],
                    attrib.vertices[3 * index.vertex_index + offset_z]
                );

                ret.vertices[i].color = glm::vec3(1.0f);

                ret.vertices[i].normal = glm::vec3(
                    attrib.normals[3 * index.normal_index + offset_x],
                    attrib.normals[3 * index.normal_index + offset_y],
                    attrib.normals[3 * index.normal_index + offset_z]
                );

                ret.vertices[i].texCoords = glm::vec2(
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                );
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
}

namespace TMP_Update {
    glm::mat4 perspective_projection;
    static float x = 0.0f;
    DataUniformFrame ubo = (DataUniformFrame){
        .light_ambient = glm::vec3(0.3f, 0.3f, 0.3f),
        .light_dir = glm::vec3(1, 1, 1),
        .light_color = glm::vec3(1, 1, 1),
        .light_intensity = 1
    };

    DataUniformMaterial tmp_material = (DataUniformMaterial){
        .ambient = 1,
        .diffuse = 1,
        .specular = 1,
        .specular_exp = 200,
    };

    const uint32_t drawcall_cout = 20;
    std::vector<DataUniformModel> model_data;

    void TMP_update_gui() {
        ImGui::Begin("tmp_update_info");

        ImGui::SeparatorText("Frame data");
        ImGui::ColorPicker3("Light Color Ambient", &ubo.light_ambient.x);
        ImGui::ColorPicker3("Light Color Light", &ubo.light_color.x);
        ImGui::DragFloat3("Light Direction", &ubo.light_dir.x);
        ImGui::DragFloat("Light Intensity", &ubo.light_intensity);

        ImGui::SeparatorText("Material Data");
        ImGui::DragFloat("Ambient", &tmp_material.ambient);
        ImGui::DragFloat("Diffuse", &tmp_material.diffuse);
        ImGui::DragFloat("Specular", &tmp_material.specular);
        ImGui::DragFloat("Specular Exponent", &tmp_material.specular_exp);

        ImGui::End();
    }

    void updateUniformBuffer(VkExtent2D swapchain_extent) {
        // zoom out depending on loaded objects
        float l = glm::sqrt(drawcall_cout);
        //float l = 2;
        float fow = (float)swapchain_extent.width / swapchain_extent.height;
        perspective_projection = glm::perspective(glm::radians(45.0f), fow, 0.1f, (float)drawcall_cout);
        //perspective_projection = glm::perspective(glm::radians(45.0f), fow, 0.1f, 10.0f);

        // TODO calculate deltaTime
        const float time = 1.0f / 60.0f;
        const float offset = 1.0f;
        x += time;


        const glm::vec3 dir_up = (glm::vec3){ 0, 0, 1 };
        const glm::vec3 pos_camera = glm::rotate(glm::mat4(1.0f), x, dir_up) * (glm::vec4) { l, l, l, 1 };
        const glm::vec3 pos_target = (glm::vec3){ 0, 0, 0 };

        ubo.view = glm::lookAt(pos_camera, pos_target, dir_up);
        ubo.proj = perspective_projection;

        // invert up axis
        ubo.proj[1][1] *= -1;
    }
}

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

        TMP_Assets::num_mesh_assets = 0;
        for (const auto& entry : std::filesystem::directory_iterator("res/models/pack_prototype"))
        {
            TMP_Assets::mesh_data[TMP_Assets::num_mesh_assets++] = TMP_Assets::load_mesh(entry.path().string().c_str(), true);
        }
        //TMP_Assets::mesh_data[TMP_Assets::num_mesh_assets++] = TMP_Assets::load_mesh("res/models/viking_room.obj");
        TMP_Assets::texture_data[0] = TMP_Assets::load_texture("res/textures/colormap.png", TMP_Assets::TEX_CHANNELS_RGB_A);
        //TMP_Assets::texture_data[0] = TMP_Assets::load_texture("res/textures/viking_room.png", TMP_Assets::TEX_CHANNELS_RGB_A);


        TMP_Update::model_data.resize(TMP_Update::drawcall_cout);
        int l = glm::sqrt(TMP_Update::drawcall_cout);
        for (int i = 0; i < TMP_Update::drawcall_cout; ++i)
            TMP_Update::model_data[i] = DataUniformModel{ .model = glm::translate(glm::mat4(1.0f), glm::vec3(i % l - l/2, i / l - l/2, 0)) };

        for(auto& data : TMP_Assets::mesh_data)
            Drawcall::createModelBuffers(data.first, device, this);

        for (auto& data : TMP_Assets::texture_data)
            Drawcall::createTextureImage(data.first, data.second, device, this);

        // only one renderpass for now
        m_render_passes.resize(1);
        m_render_passes[0] = std::make_unique<vkc::RenderPass>(device, this);
    }

    RenderContext::~RenderContext() {
        Drawcall::destroy_resources(m_device);
        vkDestroyCommandPool(m_device, m_command_pool, NULL);
    }

    const VkPhysicalDeviceProperties& RenderContext::get_physical_device_properties() const {
        return m_obj_physical_device->get_physical_device_properties();
    }

    void RenderContext::render_begin() {
        //clear previous drawcalls
        Drawcall::clear_drawcalls();
        // make up some drawcalls for testing
        TMP_Update::TMP_update_gui();
        TMP_Update::updateUniformBuffer(m_swapchain->get_extent());
        vkc::RenderPass* obj_render_pass = m_render_passes[0].get();
        for (uint32_t i = 0; i < TMP_Update::drawcall_cout; ++i)
        {
            // base drawcall
            Drawcall::add_drawcall(Drawcall::DrawcallData(
                obj_render_pass,
                obj_render_pass->get_pipeline_ptr(0),
                &TMP_Update::tmp_material,
                TMP_Update::model_data[i],
                i % TMP_Assets::num_mesh_assets)
            );
            //// outline drawcall
            //Drawcall::add_drawcall(Drawcall::DrawcallData(
            //    obj_render_pass,
            //    obj_render_pass->get_pipeline_ptr(1),
            //    &TMP_Update::tmp_material,
            //    TMP_Update::model_data[i],
            //    i % TMP_Assets::num_mesh_assets)
            //);
        }
    }

    void RenderContext::render_finalize() {
        m_frames[m_active_frame_index]->render(
            m_swapchain->get_handle(),
            m_queue_graphic,
            m_queue_present,
            m_active_frame_index,
            m_swapchain->get_extent(),
            TMP_Update::ubo,
            Drawcall::get_drawcalls()
        );

        m_active_frame_index = (m_active_frame_index + 1) % get_num_render_frames();
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
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_queue_graphic, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_queue_graphic);

        vkFreeCommandBuffers(m_device, m_command_pool, 1, &commandBuffer);
    }

    void RenderContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion = { 0 };
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void RenderContext::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region = { 0 };
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = (VkOffset3D){
            .x = 0,
            .y = 0,
            .z = 0
        };
        region.imageExtent = (VkExtent3D){
            .width = width,
            .height = height,
            .depth = 1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

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

    void RenderContext::transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

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

    void RenderContext::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* pImage, VkDeviceMemory* pImageMemory) {
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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

    VkImageView RenderContext::create_imge_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = aspectFlags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(m_device, &createInfo, NULL, &imageView) != VK_SUCCESS) {
            CC_LOG(ERROR,"failed to create image views");
        }

        return imageView;
    }
}