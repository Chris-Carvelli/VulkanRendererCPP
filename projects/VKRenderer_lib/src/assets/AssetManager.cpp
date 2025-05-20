#include "AssetManager.hpp"

#include <core/DrawCall.hpp>

// tiny obj
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
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

        // fullscreen triangle
        static glm::vec3 builtin_fullscreen_tri_vertex_data[] = {
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3( 3.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f,  3.0f, 0.0f)
        };
        static uint32_t builtin_fullscreen_tri_index_data[] = { 0, 1, 2 };

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

        static const MeshData BUILTIN_FULLSCREEN_TRI = {
            .vertex_data = builtin_fullscreen_tri_vertex_data,
            .vertex_count = sizeof(builtin_fullscreen_tri_vertex_data) / sizeof(glm::vec3),
            .vertex_data_size = sizeof(glm::vec3),
            .index_data = builtin_fullscreen_tri_index_data,
            .index_count = sizeof(builtin_fullscreen_tri_index_data) / sizeof(uint32_t)
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
        create_mesh(BuiltinPrimitives::BUILTIN_FULLSCREEN_TRI);
    }

    uint32_t get_num_mesh_assets() {
        return num_mesh_assets;
    }

    uint32_t get_num_texture_assets() {
        return num_texture_assets;
    }

    uint32_t get_num_material_assets() {
        return num_material_assets;
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

    IdAssetMesh load_mesh(const char* path) {
        return load_mesh(path, "");
    }

    void load_mesh_tinyobj(const char* path, const char* path_material, MeshData& data) {

        const int offset_x = 2;
        const int offset_y = 1;
        const int offset_z = 0;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        std::string warn;

        CC_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path, path_material), "[tinyobj] could not load %s", path);
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
                    -attrib.vertices[3 * index.vertex_index + offset_z]
                );

                p_vertex_data[i].color = glm::vec3(1.0f);


                p_vertex_data[i].normal = glm::vec3(
                    attrib.normals[3 * index.normal_index + offset_x],
                    attrib.normals[3 * index.normal_index + offset_y],
                    attrib.normals[3 * index.normal_index + offset_z]
                );

                //CC_LOG(LOG, "%0.3f, %0.3fm %0.3f", p_vertex_data[i].normal.x, p_vertex_data[i].normal.y,p_vertex_data[i].normal.z);

                p_vertex_data[i].texCoords = glm::vec2(
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                );

                ++i;
            }
    }

    // flipp X and Z to match Kenney assets orientation
    // (prob. default blender)
    // this should probably be an import util + cooking anyway
    IdAssetMesh load_mesh(const char* path, const char* path_material) {

        MeshData data;
        IdAssetMesh mesh_idx = num_mesh_assets++;

        load_mesh_tinyobj(path, path_material, data);
        //load_mesh_assimp(path, data);

        mesh_data[mesh_idx] = data;
        return mesh_idx;
    }

    void debug_print_material_info(const aiMaterial& ai_material_data) {
        // TMP print material properties
        CC_LOG(IMPORTANT, "%-28s\ttype\tsemantic\tvalue", "name");
        for (int j = 0; j < ai_material_data.mNumProperties; ++j) {
            const aiMaterialProperty& ai_mat_prop_data = *ai_material_data.mProperties[j];

            char buf[64];
            aiString path;
            if (ai_material_data.mNumAllocated < 63)
            {
                switch(ai_mat_prop_data.mType) {
                case aiPTI_Float:   sprintf(buf, "%f", (float)  *ai_mat_prop_data.mData); break;
                case aiPTI_Double:  sprintf(buf, "%f", (double) *ai_mat_prop_data.mData); break;
                case aiPTI_Integer: sprintf(buf, "%d", (int)    *ai_mat_prop_data.mData); break;
                case aiPTI_String:
                    if (ai_mat_prop_data.mSemantic != aiTextureType_NONE) {
                        ai_material_data.GetTexture(static_cast<aiTextureType>(ai_mat_prop_data.mSemantic), 0, &path);
                        memcpy(buf, path.C_Str(), path.length + 1); // we already ensured that the string is short enough
                    }
                    else
                        sprintf(buf, "%s", ai_mat_prop_data.mData);
                    break;
                case aiPTI_Buffer:  sprintf(buf, "%s", ai_mat_prop_data.mData); break;
                default:            sprintf(buf, "<unrecognized buffer type>"); break;
                }
            }
            else
                sprintf(buf, "<too much data, check in debug modw>");

            CC_LOG(VERBOSE, "%-28s\t%d\t%d\t\t%s", ai_mat_prop_data.mKey.C_Str(), ai_mat_prop_data.mType, ai_mat_prop_data.mSemantic, buf);
        }
    }

    IdAssetTexture load_tex(const aiTextureType type, const aiMaterial& mat, const TexChannelTypes channels, const VkFormat format, const std::string base_path) {
        aiString path;
        if (mat.GetTextureCount(type) == 0)
            return BuiltinPrimitives::IDX_TEX_WHITE;
        mat.GetTexture(type, 0, &path);
        path = base_path + path.C_Str();
        return vkc::Assets::load_texture(path.C_Str(), channels, vkc::Assets::TEX_VIEW_TYPE_2D, format, true);
    }

    // load all mehses and materials from OBJ or FBX file
    // at the moment, each mesh will have its own material
    uint32_t load_model(
        const char* path,
        const char* base_path_textures,
        IdAssetTexture TMP_tex_environment_id,
        std::vector<IdAssetMesh>&     out_loaded_mesh_idxs,
        std::vector<IdAssetMaterial>& out_loaded_materials_idxs
    ) {

        

        // Read the file using Assimp importer
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_CalcTangentSpace | aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
        );

        CC_ASSERT(scene, "[assimp] could not load %s", path);



        out_loaded_mesh_idxs.resize(scene->mNumMeshes);
        out_loaded_materials_idxs.resize(scene->mNumMeshes);

        // create material
        std::map<unsigned int, IdAssetMaterial> material_map;
        for(int i = 0; i < scene->mNumMaterials; ++i) {
            // load textures
            const aiMaterial& ai_material_data = *scene->mMaterials[i];

            debug_print_material_info(ai_material_data);
                        
            // TMP cannon model has unused materials that do not conform with the texture setup of
            // the PBR material. Skipping for now
            int illum_type_idx;
            CC_ASSERT(aiGetMaterialInteger(&ai_material_data, "$mat.illum", aiTextureType_NONE, 0, &illum_type_idx) == aiReturn_SUCCESS, "error retrieving assimp property $mat.illum from material %d, model %s", i, path);

            if (illum_type_idx == 1)
            {
                CC_LOG(WARNING, "unrecognized illumination model `2` for for material id %d, model %s", i, path);
                continue;
            }

            // TODO hardcoded textures
            std::string base_path(base_path_textures);

            IdAssetTexture tex_idx_arm     = load_tex(aiTextureType_SHININESS, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, VK_FORMAT_R8G8B8A8_SRGB, base_path);
            IdAssetTexture tex_idx_diffuse = load_tex(aiTextureType_DIFFUSE, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, VK_FORMAT_R8G8B8A8_SRGB, base_path);
            IdAssetTexture tex_idx_normal   = load_tex(aiTextureType_NORMALS, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB, VK_FORMAT_R8G8B8_UNORM, base_path);

            auto mat = (MaterialData) {
                .id_pipeline_config = vkc::PIPELINE_CONFIG_ID_PBR,
                .id_render_pass = 0,
                .id_pipeline = 0,
                .uniform_data_material = nullptr,
                .image_views = std::vector<IdAssetTexture> {
                    tex_idx_diffuse,
                    tex_idx_arm,
                    tex_idx_normal,
                    TMP_tex_environment_id,
                }
            };
            // TODO hardcoded PBR material
            auto tmp = create_material(mat);
            CC_LOG(VERBOSE, "mat %d: %d", i, tmp);
            material_map[i] = tmp;
        }

        for(int i = 0; i < scene->mNumMeshes; ++i) {
            IdAssetMesh mesh_idx = num_mesh_assets++;
            MeshData data { };
            const aiMesh& ai_mesh_data = *scene->mMeshes[i];

            uint64_t stride_uv = sizeof(*ai_mesh_data.mTextureCoords[0]);

            data.vertex_data_size = sizeof(VertexData);
            data.vertex_data = new VertexData[ai_mesh_data.mNumVertices];
            data.vertex_count = ai_mesh_data.mNumVertices;
            VertexData* p = (VertexData*)data.vertex_data;
            for(int j = 0; j < ai_mesh_data.mNumVertices; ++j) {
                VertexData vertex_data = { };
                p[j].position.x = ai_mesh_data.mVertices[j].x;
                p[j].position.y = ai_mesh_data.mVertices[j].y;
                p[j].position.z = ai_mesh_data.mVertices[j].z;

                if(ai_mesh_data.mColors[0] != nullptr) {
                    p[j].color.r = ai_mesh_data.mColors[0][j].r;
                    p[j].color.g = ai_mesh_data.mColors[0][j].g;
                    p[j].color.b = ai_mesh_data.mColors[0][j].b;
                }

                if(ai_mesh_data.mTextureCoords[0] != nullptr) {
                    p[j].texCoords.x = ai_mesh_data.mTextureCoords[0][j].x;
                    p[j].texCoords.y = ai_mesh_data.mTextureCoords[0][j].y;
                }

                if(ai_mesh_data.mNormals != nullptr) {
                    p[j].normal.x = ai_mesh_data.mNormals[j].x;
                    p[j].normal.y = ai_mesh_data.mNormals[j].y;
                    p[j].normal.z = ai_mesh_data.mNormals[j].z;
                }
                if(ai_mesh_data.mTangents != nullptr) {
                    p[j].tangent.x = ai_mesh_data.mTangents[j].x;
                    p[j].tangent.y = ai_mesh_data.mTangents[j].y;
                    p[j].tangent.z = ai_mesh_data.mTangents[j].z;
                }

            }

            // this is a conservative estimate, we may have way less
            uint32_t num_indices = ai_mesh_data.mNumFaces * 3;

            // allocate worst case scenario
            data.index_data = (uint32_t*)malloc(sizeof(uint32_t) * num_indices);
            data.index_count = num_indices;
            uint32_t idx = 0;
            for(int j = 0; j < ai_mesh_data.mNumFaces; ++j) {
                data.index_data[idx + 0] = ai_mesh_data.mFaces[j].mIndices[0];
                data.index_data[idx + 1] = ai_mesh_data.mFaces[j].mIndices[1];
                data.index_data[idx + 2] = ai_mesh_data.mFaces[j].mIndices[2];

                idx += 3;
            }

           mesh_data[mesh_idx] = data;
           out_loaded_mesh_idxs[i] = mesh_idx;
           out_loaded_materials_idxs[i] = material_map[ai_mesh_data.mMaterialIndex];
           CC_LOG(IMPORTANT, "file: %d, actual: %d", ai_mesh_data.mMaterialIndex, out_loaded_materials_idxs[i]);
        }

        return scene->mNumMeshes;
    }

    std::vector<IdAssetMesh> load_meshes(const char** paths) {
        return {}; // TODO
    }

    std::vector<IdAssetMesh> load_meshes_from_folder(const char* folder_path) {
        std::vector<IdAssetMesh> ret = std::vector<IdAssetMesh>();
        for (const auto& entry : std::filesystem::directory_iterator(folder_path))
        {
            ret.push_back(load_mesh(entry.path().string().c_str()));
            // TMP
            break;
        }

        return ret;
    }

    IdAssetTexture load_texture(const char* path, TexChannelTypes channels, TexViewTypes viewType, VkFormat format, bool flip_vertical) {
        const uint8_t requested_channels_count = tex_get_num_channels(channels);

        int texWidth = 0;
        int texHeight = 0;
        int texChannels = 0;

        stbi_set_flip_vertically_on_load(flip_vertical ? 1 : 0);
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, requested_channels_count);

        if (pixels == nullptr) {
            CC_LOG(WARNING, "missing texture at path %s", path);
            return IDX_MISSING_TEXTURE;
        }

        // TODO get number of ACTUAL channels based on requested channels (`channels` parameter)
        size_t size = texWidth * texHeight * requested_channels_count;

        IdAssetTexture tex_idx = num_texture_assets++;
        TextureData data;
        data.viewType = (VkImageViewType)viewType;
        data.width = (uint16_t)texWidth;
        data.height = (uint16_t)texHeight;
        data.channelsCount = (uint8_t)requested_channels_count;
        data.channels = channels;
        data.format = format;
        data.data.resize(size);
        memcpy(data.data.data(), pixels, size);

        stbi_image_free(pixels);

        if (viewType == TEX_VIEW_TYPE_CUBE) {
            data.width /= 4;
            data.height /= 3;
        }
        texture_data[tex_idx] = data;
        return tex_idx;
    }

    IdAssetMesh create_mesh(MeshData data) {
        data.flags |= MeshData::FLAG_DYNAMIC;

        IdAssetMesh mesh_idx = num_mesh_assets++;
        mesh_data[mesh_idx] = data;
        return mesh_idx;
    }

    IdAssetMaterial create_material(MaterialData& data) {
        IdAssetMaterial material_idx = num_material_assets++;
        //memcpy(&material_data[material_idx], &data, sizeof(MaterialData));
        material_data[material_idx] = data;
        return material_idx;
    }
}