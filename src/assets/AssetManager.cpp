#include "AssetManager.hpp"

#include <core/DrawCall.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <stb_image.h>

#include <map>
#include <filesystem>
#include <string.h>

namespace vkc::Assets {
    namespace BuiltinPrimitives {
        //// filled cube with triangle topology
        /*static uint32_t debug_cube_index_data[] = {
            0, 1, 2, 2, 3, 0,
            0, 1, 2, 2, 3, 0,
            0, 1, 5, 5, 4, 0,
            1, 2, 6, 6, 5, 1,
            2, 3, 7, 7, 6, 2,
            3, 0, 4, 4, 7, 3
        };*/

        static glm::vec3 debug_cube_vertex_data[] = {
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3(0.5f, -0.5f, -0.5f),
            glm::vec3(0.5f, -0.5f,  0.5f),
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),
            glm::vec3(0.5f,  0.5f, -0.5f),
            glm::vec3(0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f),
        };

        // indices for line topology
        static uint32_t debug_cube_index_data[] = {
            0, 1, 1, 2, 2, 3, 3, 0, // bottom
            4, 5, 5, 6, 6, 7, 7, 4, // top
            0, 4, 1, 5, 2, 6, 3, 7  // sides
        };

        static glm::vec3 debug_ray_vertex_data[] = {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
        };

        // indices for line topology
        static uint32_t debug_ray_index_data[] = { 0, 1 };


        static const MeshData DEBUG_CUBE_MESH_DATA = {
            .vertex_data = debug_cube_vertex_data,
            .vertex_count = sizeof(debug_cube_vertex_data) / sizeof(glm::vec3),
            .vertex_data_size = sizeof(glm::vec3),
            .index_data = debug_cube_index_data,
            .index_count = sizeof(debug_cube_index_data) / sizeof(uint32_t)
        };

        static const MeshData DEBUG_RAY_MESH_DATA = {
            .vertex_data = debug_ray_vertex_data,
            .vertex_count = sizeof(debug_ray_vertex_data) / sizeof(glm::vec3),
            .vertex_data_size = sizeof(glm::vec3),
            .index_data = debug_ray_index_data,
            .index_count = sizeof(debug_ray_index_data) / sizeof(uint32_t)
        };
    }

    uint32_t num_mesh_assets = 0;
    uint32_t num_texture_assets = 0;
    uint32_t num_material_assets = 0;

    std::map<IdAssetMesh, MeshData> mesh_data;
    std::map<IdAssetTexture, TextureData> texture_data;
    std::map<IdAssetMaterial, MaterialData> material_data;

    void asset_manager_init() {
        // create assets on CPU
        create_mesh(BuiltinPrimitives::DEBUG_CUBE_MESH_DATA);
        create_mesh(BuiltinPrimitives::DEBUG_RAY_MESH_DATA);
    }

    uint32_t get_num_mesh_assets() {
        return num_mesh_assets;
    }

    uint32_t get_num_texture_assets() {
        return num_texture_assets;
    }

    MeshData& get_mesh_data(IdAssetMesh id) {
        return mesh_data[id];
    }

    TextureData& get_texture_data(IdAssetTexture id) {
        return texture_data[id];
    }

    MaterialData& get_material_data(IdAssetMaterial id) {
        return material_data[id];
    }

    // flipp X and Y to match Kenney assets orientation
    // (prob. default blender)
    // this should probably be an import util + cooking anyway
    IdAssetMesh load_mesh(const char* path) {
        const int offset_x = 0;
        const int offset_y = 1;
        const int offset_z = 2;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        std::string warn;

        assert(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path));

        MeshData data;
        IdAssetMesh mesh_idx = num_mesh_assets++;

        int n_indices = 0;
        // cunt indices (vertices will be the same number because we're cutting corners)
        for (const auto& shape : shapes)
            n_indices += shape.mesh.indices.size();

        data.vertex_data_size = sizeof(VertexData);
        data.vertex_count = n_indices;
        data.index_count = n_indices;

        // TMP no need for full semantics yet.
        // We will assume that all meshes from file have the same format,
        // and have custom ones only for debug and procecural mehses
        data.vertex_data = new VertexData[n_indices];
        data.index_data = new uint32_t[n_indices];

        // TODO RIGHTNOW
        // - [x] allocate vertex and index data 
        // - [ ] free vertex and index data on asset destruction
        // - [x] update usages of MeshData in `RenderFrame`

        int i = 0;
        VertexData* p_vertex_data = (VertexData*)data.vertex_data;
        for (const auto& shape : shapes)
            for (const auto& index : shape.mesh.indices) {
                data.index_data[i] = i;
                p_vertex_data[i].position = glm::vec3(
                    attrib.vertices[3 * index.vertex_index + offset_x],
                    attrib.vertices[3 * index.vertex_index + offset_y],
                    attrib.vertices[3 * index.vertex_index + offset_z]
                );

                p_vertex_data[i].color = glm::vec3(1.0f);

                p_vertex_data[i].normal = glm::vec3(
                    attrib.normals[3 * index.normal_index + offset_x],
                    attrib.normals[3 * index.normal_index + offset_y],
                    attrib.normals[3 * index.normal_index + offset_z]
                );

                p_vertex_data[i].texCoords = glm::vec2(
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                );
                ++i;
            }

        mesh_data[mesh_idx] = data;
        return mesh_idx;
    }

    std::vector<IdAssetMesh> load_meshes(const char** paths) {
        return {}; // TODO
    }

    std::vector<IdAssetMesh> load_meshes_from_folder(const char* folder_path) {
        std::vector<IdAssetMesh> ret = std::vector<IdAssetMesh>();
        for (const auto& entry : std::filesystem::directory_iterator(folder_path))
            ret.push_back(load_mesh(entry.path().string().c_str()));

        return ret;
    }

    IdAssetTexture load_texture(const char* path, TexChannelTypes channels) {
        int texWidth = 0;
        int texHeight = 0;
        int texChannels = 0;
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, channels);
        assert(pixels != nullptr);

        // TODO get number of ACTUAL channels based on requested channels (`channels` parameter)
        size_t size = texWidth * texHeight * 4;

        IdAssetTexture tex_idx = num_texture_assets++;
        TextureData data;
        data.width = (uint16_t)texWidth;
        data.height = (uint16_t)texHeight;
        data.channelsCount = (uint8_t)texChannels;
        data.channels = channels;
        data.data.resize(size);
        memcpy(data.data.data(), pixels, size);

        stbi_image_free(pixels);

        texture_data[tex_idx] = data;
        return tex_idx;
    }

    IdAssetMesh create_mesh(MeshData data) {
        IdAssetMesh mesh_idx = num_mesh_assets++;
        mesh_data[mesh_idx] = data;
        return mesh_idx;
    }

    IdAssetMaterial create_material(MaterialData data) {
        IdAssetMaterial material_idx = num_material_assets++;
        material_data[material_idx] = data;
        return material_idx;
    }
}