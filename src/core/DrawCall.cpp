#include "DrawCall.hpp"

#include <vector>
#include <map>

namespace vkc::Drawcall {
    std::map<uint32_t, ModelDataGPU> model_data_gpu;
    std::map<uint32_t, TextureDataGPU> texture_data_gpu;
    std::vector<DrawcallData> drawcalls;

    void add_drawcall(DrawcallData data) {
        drawcalls.push_back(data);
    }

    const std::vector<DrawcallData>& get_drawcalls() {
        return drawcalls;
    }

    void clear_drawcalls() {
        drawcalls.clear();
    }

    VkImageView get_texture_image_view(uint32_t id) {
        return texture_data_gpu[id].image_view;
    }

    ModelDataGPU get_model_data(uint32_t index) {
        return model_data_gpu[index];
    }

    // only one texture for now
    void createTextureImage(uint32_t texture_id, TMP_Assets::TextureData& texture_data, VkDevice device, vkc::RenderContext* obj_render_context) {
        TextureDataGPU new_gpu_data = { 0 };

        VkDeviceSize imageSize = texture_data.width * texture_data.height * 4;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        obj_render_context->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer,
            &stagingBufferMemory
        );

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, texture_data.data.data(), imageSize);
        vkUnmapMemory(device, stagingBufferMemory);

        // // no free, we can't fit all our scene in GPU memory at the same time
        //res_tex_free(textureId);

        obj_render_context->create_image(
            texture_data.width,
            texture_data.height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &new_gpu_data.image,
            &new_gpu_data.image_memory
        );

        obj_render_context->transition_image_layout(
            new_gpu_data.image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        obj_render_context->copy_buffer_to_image(
            stagingBuffer,
            new_gpu_data.image,
            texture_data.width,
            texture_data.height
        );

        obj_render_context->transition_image_layout(
            new_gpu_data.image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        vkDestroyBuffer(device, stagingBuffer, NULL);
        vkFreeMemory(device, stagingBufferMemory, NULL);

        // view
        new_gpu_data.image_view = obj_render_context->create_imge_view(
            new_gpu_data.image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        texture_data_gpu[texture_id] = new_gpu_data;
    }

    // createIndexBuffers
    // createVertexBuffers
    void createModelBuffers(uint32_t model_index, VkDevice device, vkc::RenderContext* obj_render_context) {
        TMP_Assets::MeshData& mesh_data = TMP_Assets::get_mesh_data(model_index);
        model_data_gpu[model_index] = ModelDataGPU();

        ModelDataGPU& model_data_gpu_ref = model_data_gpu[model_index];


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
                &model_data_gpu_ref.vertex_buffer,
                &model_data_gpu_ref.vertexbuffer_memory
            );

            obj_render_context->copyBuffer(stagingBuffer, model_data_gpu_ref.vertex_buffer, bufferSize);

            vkDestroyBuffer(device, stagingBuffer, NULL);
            vkFreeMemory(device, stagingBufferMemory, NULL);
        }


        // indices  ============================================================
        {
            model_data_gpu_ref.indices_count = mesh_data.indices.size();
            // TODO FIXME this won't work if we don't use indices.
            // but we can't just sizeof(indices), since now indices is now a pointer variable
            VkDeviceSize bufferSize = sizeof(mesh_data.indices[0]) * mesh_data.indices.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            obj_render_context->createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer,
                &stagingBufferMemory
            );

            // map memory
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, mesh_data.indices.data(), (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            obj_render_context->createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &model_data_gpu_ref.index_buffer,
                &model_data_gpu_ref.index_buffer_memory
            );

            obj_render_context->copyBuffer(
                stagingBuffer,
                model_data_gpu_ref.index_buffer,
                bufferSize
            );

            vkDestroyBuffer(device, stagingBuffer, NULL);
            vkFreeMemory(device, stagingBufferMemory, NULL);
        }
    }

    void destroy_resources(VkDevice device) {
        for (auto& data : texture_data_gpu)
        {
            vkDestroyImageView(device, data.second.image_view, NULL);
            vkDestroyImage(device, data.second.image, NULL);
            vkFreeMemory(device, data.second.image_memory, NULL);
        }

        for (auto& data : model_data_gpu)
        {
            vkDestroyBuffer(device, data.second.vertex_buffer, NULL);
            vkDestroyBuffer(device, data.second.index_buffer, NULL);
            vkFreeMemory(device, data.second.index_buffer_memory, NULL);
            vkFreeMemory(device, data.second.vertexbuffer_memory, NULL);
        }
    }
}