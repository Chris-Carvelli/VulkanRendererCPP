#include "DrawCall.hpp"

#include <VulkanUtils.h>

// debug draw calls includes
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

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
    void createTextureImage(Assets::IdAssetTexture texture_id, VkDevice device, vkc::RenderContext* obj_render_context) {
        const Assets::TextureData& texture_data = Assets::get_texture_data(texture_id);

        // TODO FIXME clean up texture creation pipeline
        if (texture_data.viewType == VK_IMAGE_VIEW_TYPE_CUBE)
        {
            createTextureCubemap(texture_id, device, obj_render_context);
            return;
        }
        TextureDataGPU new_gpu_data = { 0 };

        VkDeviceSize imageSize = texture_data.data.size();

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
            texture_data.viewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1,
            texture_data.mipmaps,
            static_cast<VkFormat>(texture_data.format),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &new_gpu_data.image,
            &new_gpu_data.image_memory
        );

        obj_render_context->transition_image_layout(
            new_gpu_data.image,
            1,
            texture_data.mipmaps,
            static_cast<VkFormat>(texture_data.format),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        obj_render_context->copy_buffer_to_image(
            stagingBuffer,
            new_gpu_data.image,
            texture_data
        );

        obj_render_context->transition_image_layout(
            new_gpu_data.image,
            1,
            texture_data.mipmaps,
            static_cast<VkFormat>(texture_data.format),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        vkDestroyBuffer(device, stagingBuffer, NULL);
        vkFreeMemory(device, stagingBufferMemory, NULL);

        // view
        new_gpu_data.image_view = obj_render_context->create_imge_view(
            new_gpu_data.image,
            static_cast<VkFormat>(texture_data.format),
            VK_IMAGE_ASPECT_COLOR_BIT,
            static_cast<VkImageViewType>(texture_data.viewType),
            texture_data.mipmaps
        );

        texture_data_gpu[texture_id] = new_gpu_data;
    }

    void createTextureCubemap(Assets::IdAssetTexture texture_id, VkDevice device, vkc::RenderContext* obj_render_context) {
        const uint32_t NUM_FACES_TOT = 6;
        const uint32_t NUM_FACES_ROW = 3;
        const uint32_t NUM_FACES_COL = 4;
        const uint32_t SIZE_PIXEL = 4;
        /*
        const uint8_t  IDX_FACE_X[] = { 0, 2, 1, 1, 3, 1 };
        const uint8_t  IDX_FACE_Y[] = { 1, 1, 2, 0, 1, 1 };*/
        //                shoudl be   : L  R  D  U, B
        //                cubemap side: R  L  U  D, F
        const uint8_t  IDX_FACE_X[] = { 2, 0, 1, 1, 1, 3 };
        const uint8_t  IDX_FACE_Y[] = { 1, 1, 0, 2, 1, 1 };


        const Assets::TextureData& texture_data = Assets::get_texture_data(texture_id);
        
        TextureDataGPU new_gpu_data = { 0 };

        uint32_t pixel_count_row   = texture_data.width;
        uint32_t pixel_count_layer = pixel_count_row * pixel_count_row;
        uint64_t stride = pixel_count_row * NUM_FACES_COL * SIZE_PIXEL;
        VkDeviceSize size_row   = pixel_count_row   * SIZE_PIXEL;
        VkDeviceSize size_layer = pixel_count_layer * SIZE_PIXEL;
        VkDeviceSize size_image = size_layer * NUM_FACES_TOT;
        VkDeviceSize size_source_image = texture_data.width * NUM_FACES_COL * texture_data.height * NUM_FACES_ROW * SIZE_PIXEL;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        obj_render_context->createBuffer(
            size_image,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer,
            &stagingBufferMemory
        );

        int TMP_C = 0;
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, size_image, 0, &data);
        unsigned char* dst = (unsigned char*)data;
        const unsigned char* src = texture_data.data.data();
        for(int i = 0; i < NUM_FACES_TOT; ++i)
        {
            VkDeviceSize dst_offset = size_layer * i;
            VkDeviceSize src_offset = IDX_FACE_Y[i] * stride * pixel_count_row + IDX_FACE_X[i] * pixel_count_row * SIZE_PIXEL;

            for(int j = 0; j < pixel_count_row; ++j)
            {
                assert(src_offset + size_row <= size_source_image);
                assert(dst_offset + size_row <= size_image);
                TMP_C++;
                memcpy(dst + dst_offset, src + src_offset, size_row);
                dst_offset += size_row;
                src_offset += stride;
            }
        }
        vkUnmapMemory(device, stagingBufferMemory);
        

        // // no free, we can't fit all our scene in GPU memory at the same time
        //res_tex_free(textureId);

        obj_render_context->create_image(
            texture_data.width,
            texture_data.height,
            texture_data.viewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1,
            texture_data.mipmaps,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &new_gpu_data.image,
            &new_gpu_data.image_memory
        );

        obj_render_context->transition_image_layout(
            new_gpu_data.image,
            6,
            texture_data.mipmaps,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        obj_render_context->copy_buffer_to_image(
            stagingBuffer,
            new_gpu_data.image,
            texture_data
        );

        obj_render_context->transition_image_layout(
            new_gpu_data.image,
            6,
            texture_data.mipmaps,
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
            VK_IMAGE_ASPECT_COLOR_BIT,
            static_cast<VkImageViewType>(texture_data.viewType),
            texture_data.mipmaps
        );

        texture_data_gpu[texture_id] = new_gpu_data;
    }

    void updateModelVertexBuffer(
        uint32_t model_index,
        void* vertex_buffer_content,
        uint32_t vertex_buffer_size,
        VkDevice device,
        vkc::RenderContext* obj_render_context
    ) {
        ModelDataGPU& model_data_gpu_ref = model_data_gpu[model_index];

        {
            VkDeviceSize bufferSize = vertex_buffer_size;

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
            memcpy(data, vertex_buffer_content, (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            obj_render_context->copyBuffer(
                stagingBuffer,
                model_data_gpu_ref.vertex_buffer,
                bufferSize);

            vkDestroyBuffer(device, stagingBuffer, NULL);
            vkFreeMemory(device, stagingBufferMemory, NULL);
        }
    }

    uint32_t createModelBuffers(uint32_t model_index, VkDevice device, vkc::RenderContext* obj_render_context) {
        Assets::MeshData& mesh_data = Assets::get_mesh_data(model_index);

        assert(mesh_data.vertex_count > 0);

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
            VkDeviceSize bufferSize = mesh_data.vertex_count * mesh_data.vertex_data_size;

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
            memcpy(data, mesh_data.vertex_data, (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            obj_render_context->createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &model_data_gpu_ref.vertex_buffer,
                &model_data_gpu_ref.vertexbuffer_memory
            );

            obj_render_context->copyBuffer(
                stagingBuffer,
                model_data_gpu_ref.vertex_buffer,
                bufferSize
            );

            vkDestroyBuffer(device, stagingBuffer, NULL);
            vkFreeMemory(device, stagingBufferMemory, NULL);

            // debug
            std::string debug_name = "vertex buffer " + std::to_string(model_index);
            vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
                (uint64_t)model_data_gpu_ref.vertex_buffer,
                VK_OBJECT_TYPE_BUFFER,
                obj_render_context->get_device(),
                debug_name.c_str()
            );
            /*std::string debug_name_2 = "staging vertex buffer " + std::to_string(model_index);
            vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
            (uint64_t)stagingBuffer,
            VK_OBJECT_TYPE_BUFFER,
            obj_render_context->get_device(),
            debug_name.c_str()
            );*/
            std::string debug_name_3 = "vertex buffer memory " + std::to_string(model_index);
            vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
                (uint64_t)model_data_gpu_ref.vertexbuffer_memory,
                VK_OBJECT_TYPE_DEVICE_MEMORY,
                obj_render_context->get_device(),
                debug_name_3.c_str()
            );
        }


        // indices  ============================================================
        {
            model_data_gpu_ref.indices_count = mesh_data.index_count;
            // TODO FIXME this won't work if we don't use indices.
            // but we can't just sizeof(indices), since now indices is now a pointer variable
            VkDeviceSize bufferSize = mesh_data.index_count * sizeof(mesh_data.index_data[0]);

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
            memcpy(data, mesh_data.index_data, (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            obj_render_context->createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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


            // debug
            std::string debug_name = "index buffer " + std::to_string(model_index);
            vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
                (uint64_t)model_data_gpu_ref.index_buffer,
                VK_OBJECT_TYPE_BUFFER,
                obj_render_context->get_device(),
                debug_name.c_str()
            );
            /*std::string debug_name_2 = "staging index buffer " + std::to_string(model_index);
            vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
            (uint64_t)stagingBuffer,
            VK_OBJECT_TYPE_BUFFER,
            obj_render_context->get_device(),
            debug_name.c_str()
            );*/
            std::string debug_name_3 = "index buffer memory " + std::to_string(model_index);
            vkc::Instance::TMP_get_singleton_instance()->add_object_debug_name(
                (uint64_t)model_data_gpu_ref.index_buffer_memory,
                VK_OBJECT_TYPE_DEVICE_MEMORY,
                obj_render_context->get_device(),
                debug_name_3.c_str()
            );
        }

        return model_index;
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

    void add_debug_name(uint32_t gpu_data_id, VkDevice device, vkc::Instance* obj_instance, const char * debug_name) {
        
        auto& gpu_data = model_data_gpu[gpu_data_id];

        obj_instance->add_object_debug_name((uint64_t)gpu_data.vertex_buffer, VK_OBJECT_TYPE_BUFFER, device, debug_name);
        obj_instance->add_object_debug_name((uint64_t)gpu_data.index_buffer, VK_OBJECT_TYPE_BUFFER, device, debug_name);
    }


    // ======================================================================
    // debug drawcalls
    // ======================================================================

    std::vector<DebugDrawcallData> debug_drawcalls;

    void add_debug_cube(glm::vec3 pos, glm::vec3 rot, glm::vec3 size, glm::vec3 color) {
        glm::mat4 mtx = glm::translate(pos) * glm::eulerAngleXYZ(rot.x, rot.y, rot.z) * glm::scale(size);
        DebugDrawcallData data = {
            .data_uniform_model = {.model = mtx, .color = color},
            .idx_data_attributes = Assets::BuiltinPrimitives::IDX_DEBUG_CUBE,
        };
        debug_drawcalls.push_back(data);
    }

    void add_debug_ray(glm::vec3 pos, glm::vec3 dir, float length, glm::vec3 color) {
        dir = glm::normalize(dir) * length;
        glm::vec3 u = glm::vec3(0, -dir.z, dir.y);
        glm::vec3 w = glm::cross(dir, u);
        glm::mat4 mtx = glm::mat4(0.0f);
        mtx[0] = glm::vec4(u, 0.0f);
        mtx[1] = glm::vec4(w, 0.0f);
        mtx[2] = glm::vec4(dir, 0.0f);
        mtx[3] = glm::vec4(pos, 1.0f);

        DebugDrawcallData data = {
            .data_uniform_model = {.model = mtx, .color = color},
            .idx_data_attributes = Assets::BuiltinPrimitives::IDX_DEBUG_RAY,
        };
        debug_drawcalls.push_back(data);
    }


    const std::vector<DebugDrawcallData>& get_debug_drawcalls() {
        return debug_drawcalls;
    }

    void clear_debug_drawcalls() {
        debug_drawcalls.clear();
    }
}