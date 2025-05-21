#include "AssetManager.hpp"

// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>

#include <map>
#include <string.h>

namespace vkc::Assets {
    // private creation/loading API, to allow creating assets with specific IDs
    // used for built-in and debug aasets, until we have multime asset storages
    void create_mesh(const IdAssetMesh id, const MeshData& data);
    void create_material(const IdAssetMaterial id, const MaterialData& data);
    IdAssetTexture load_texture(const IdAssetTexture id, const char* path, TexChannelTypes channels, TexViewTypes viewType = TEX_VIEW_TYPE_2D, TexFormat format = TEX_FORMAT_RGB_A, bool flip_vertical=false);

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
        create_mesh(BuiltinPrimitives::IDX_DEBUG_CUBE, BuiltinPrimitives::DEBUG_CUBE_MESH_DATA);
        create_mesh(BuiltinPrimitives::IDX_DEBUG_RAY, BuiltinPrimitives::DEBUG_RAY_MESH_DATA);
        create_mesh(BuiltinPrimitives::IDX_FULLSCREEN_TRI, BuiltinPrimitives::BUILTIN_FULLSCREEN_TRI);

        load_texture(BuiltinPrimitives::IDX_TEX_WHITE, "res/textures/tex_white.png", vkc::Assets::TEX_CHANNELS_RGB_A);
        load_texture(BuiltinPrimitives::IDX_TEX_BLACK, "res/textures/tex_black.png", vkc::Assets::TEX_CHANNELS_RGB_A);
        load_texture(BuiltinPrimitives::IDX_TEX_BLUE_NORM, "res/textures/tex_blue_norm.png", vkc::Assets::TEX_CHANNELS_RGB, vkc::Assets::TEX_VIEW_TYPE_2D, vkc::Assets::TexFormat::TEX_FORMAT_NORM);
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

    IdAssetTexture load_tex(const aiTextureType type, const aiMaterial& mat, const TexChannelTypes channels, const TexFormat format, IdAssetTexture tex_fallback, const std::string base_path) {
        aiString path;
        if (mat.GetTextureCount(type) == 0)
            return tex_fallback;
        mat.GetTexture(type, 0, &path);


        // TMP fix backslashes
        std::string TMP(path.C_Str());
        std::replace(TMP.begin(), TMP.end(), '\\', '/');

        path = base_path + TMP;
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

        CC_LOG(IMPORTANT, "Loading model %s...", path);

        // Read the file using Assimp importer
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_CalcTangentSpace | aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
        );

        CC_ASSERT(scene, "[assimp] could not load %s", path);
        CC_LOG(LOG, "assimp scene loaded");
        CC_LOG(LOG, "materials: %d", scene->mNumMaterials);
        CC_LOG(LOG, "meshes: %d", scene->mNumMeshes);


        out_loaded_mesh_idxs.resize(scene->mNumMeshes);
        out_loaded_materials_idxs.resize(scene->mNumMeshes);

        // create material
        std::map<unsigned int, IdAssetMaterial> material_map;
        for(int i = 0; i < scene->mNumMaterials; ++i) {
            // load textures
            const aiMaterial& ai_material_data = *scene->mMaterials[i];

            if (i < 3)
                debug_print_material_info(ai_material_data);
                        
            // TMP cannon model has unused materials that do not conform with the texture setup of
            // the PBR material. Skipping for now
            int illum_type_idx;


            //// ad-hoc check for models taken from ituGL
            //{
            //    if(aiGetMaterialInteger(&ai_material_data, "$mat.illum", aiTextureType_NONE, 0, &illum_type_idx) != aiReturn_SUCCESS)
            //    {
            //        CC_LOG(WARNING, "error retrieving assimp property $mat.illum from material %d, model %s", i, path);
            //        continue;
            //    }

            //    if (illum_type_idx == 1)
            //    {
            //        CC_LOG(WARNING, "unrecognized illumination model `2` for for material id %d, model %s", i, path);
            //        continue;
            //    }
            //}

            // TODO hardcoded textures
            std::string base_path(base_path_textures);

            IdAssetTexture tex_idx_arm     = load_tex(aiTextureType_SHININESS, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, TEX_FORMAT_RGB_A, BuiltinPrimitives::IDX_TEX_BLACK, base_path);
            IdAssetTexture tex_idx_diffuse = load_tex(aiTextureType_DIFFUSE, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, TEX_FORMAT_RGB_A, BuiltinPrimitives::IDX_TEX_WHITE, base_path);
            IdAssetTexture tex_idx_normal  = load_tex(aiTextureType_NORMALS, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB, TEX_FORMAT_NORM, BuiltinPrimitives::IDX_TEX_BLUE_NORM, base_path);

            auto mat = (MaterialData) {
                .id_pipeline_config = 1,
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
            CC_LOG(VERBOSE, "loading materials %d/%d", i+1, scene->mNumMaterials);
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
           CC_LOG(VERBOSE, "loading mesh %d/%d", i+1, scene->mNumMeshes);
        }

        return scene->mNumMeshes;
    }

    IdAssetTexture load_texture(const char* path, TexChannelTypes channels, TexViewTypes viewType, TexFormat format, bool flip_vertical) {
        IdAssetTexture tex_idx = num_texture_assets;
        
        if(load_texture(tex_idx, path, channels, viewType, format, flip_vertical) == IDX_MISSING_TEXTURE)
            return IDX_MISSING_TEXTURE;

        ++num_texture_assets;
        return tex_idx;
    }

    inline IdAssetMesh create_mesh(MeshData data) {

        IdAssetMesh mesh_idx = num_mesh_assets++;
        create_mesh(mesh_idx, data);
        return mesh_idx;
    }

    inline IdAssetMaterial create_material(MaterialData& data) {
        IdAssetMaterial material_idx = num_material_assets++;
        create_material(material_idx, data);
        return material_idx;
    }

    // ===================================================================================
    // serialization
    // ===================================================================================
    void asset_db_dump(const char *path) {
        FILE* fp = fopen(path, "wb+");
        
        CC_ASSERT(fp, "error opening file");

        uint32_t sizes[] = {
            num_mesh_assets,
            num_texture_assets,
            num_material_assets
        };

        fwrite(sizes, sizeof(uint32_t), 3, fp);

        for(auto& kp : mesh_data) {
            IdAssetMesh id = kp.first;
            MeshData& data = kp.second;
            fwrite(&id,              sizeof(IdAssetMesh), 1,                 fp);
            fwrite(&data,            sizeof(MeshData),    1,                 fp);
            fwrite(data.vertex_data, sizeof(VertexData),  data.vertex_count, fp);
            fwrite(data.index_data,  sizeof(uint32_t),    data.index_count,  fp);
        }

        for(auto& kp : texture_data) {
            IdAssetTexture id = kp.first;
            TextureData& data = kp.second;
            fwrite(&id,              sizeof(IdAssetTexture), 1,                fp);
            fwrite(&data,            sizeof(TextureData) - sizeof(std::vector<unsigned char>),    1,                fp);
            // annoying, we have to manually write size if we use std
            size_t num_bytes = data.data.size();
            fwrite(&num_bytes, sizeof(size_t), 1, fp);
            fwrite(data.data.data(), sizeof(unsigned char),  data.data.size(), fp);
        }

        for(auto& kp : material_data) {
            IdAssetMaterial id = kp.first;
            MaterialData& data = kp.second;
            fwrite(&id,                     sizeof(IdAssetMaterial), 1,                       fp);
            fwrite(&data,                   sizeof(MaterialData) - sizeof(std::vector<IdAssetTexture>),    1,                       fp);
            // annoying, we have to manually write size if we use std::vector
            size_t num_views = data.image_views.size();
            fwrite(&num_views, sizeof(size_t), 1, fp);
            fwrite(data.image_views.data(), sizeof(IdAssetTexture),  data.image_views.size(), fp);
        }

        fclose(fp);
        CC_LOG_SYS_ERROR();
        //CC_ASSERT(fclose(fp) == 0, "error closing file");
    }
    void asset_db_load(const char *path) {
        FILE* fp = fopen(path, "rb+");

        CC_ASSERT(fp, "error opening file");

        uint32_t sizes[3];

        fread(sizes, sizeof(uint32_t), 3, fp);
        num_mesh_assets     = sizes[0];
        num_texture_assets  = sizes[1];
        num_material_assets = sizes[2];

        for(int i = 0; i < num_mesh_assets; ++i) {
            IdAssetMesh id;
            MeshData data;
            fread(&id,              sizeof(IdAssetMesh), 1,                 fp);
            fread(&data,            sizeof(MeshData),    1,                 fp);
            data.vertex_data = new VertexData[data.vertex_count];
            data.index_data  = new uint32_t[data.index_count];
            fread(data.vertex_data, sizeof(VertexData),  data.vertex_count, fp);
            fread(data.index_data,  sizeof(uint32_t),    data.index_count,  fp);

            mesh_data[id] = data;
        }

        for(int i = 0; i < num_texture_assets; ++i) {
            IdAssetTexture id;
            TextureData data;
            fread(&id,              sizeof(IdAssetTexture), 1,                fp);
            fread(&data,            sizeof(TextureData) - sizeof(std::vector<unsigned char>),    1,                fp);
            // annoying, we have to manually read size if we use std::vector
            size_t num_bytes;
            fread(&num_bytes, sizeof(size_t), 1, fp);
            data.data.resize(num_bytes);
            fread(data.data.data(), sizeof(unsigned char), data.data.size(), fp);

            texture_data[id] = data;
        }

        for(int i = 0; i < num_material_assets; ++i) {
            IdAssetMaterial id;
            MaterialData data;
            fread(&id,                     sizeof(IdAssetMaterial), 1,                       fp);
            fread(&data,                   sizeof(MaterialData) - sizeof(std::vector<IdAssetTexture>),    1,                       fp);
            // annoying, we have to manually read size if we use std::vector
            size_t num_views;
            fread(&num_views, sizeof(size_t), 1, fp);
            data.image_views.resize(num_views);
            fread(data.image_views.data(), sizeof(IdAssetTexture), data.image_views.size(), fp);

            material_data[id] = data;
        }
        fclose(fp);
        CC_LOG_SYS_ERROR();
        //CC_ASSERT(fclose(fp) == 0, "error closing file");
    }

    // ===================================================================================
    // private
    // ===================================================================================
    inline void create_mesh(const IdAssetMesh id, const MeshData& data) {
        mesh_data[id] = data;
    }

    inline void create_material(const IdAssetMaterial id, const MaterialData& data) {
        material_data[id] = data;
    }

    IdAssetTexture load_texture(IdAssetTexture id, const char* path, TexChannelTypes channels, TexViewTypes viewType, TexFormat format, bool flip_vertical) {
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


        size_t size = texWidth * texHeight * requested_channels_count;

        TextureData data;
        data.viewType = viewType;
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
        texture_data[id] = data;
        return id;
    }
}