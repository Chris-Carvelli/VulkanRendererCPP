#include "AssetManager.hpp"
#include "dds.hpp"

// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>

#include <map>
#include <filesystem> // for getting file extensions

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

        // quad
        static VertexDataUnlit builtin_quad_vertex_data[] = {
            { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 0.0f) },
            { glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 0.0f) },
            { glm::vec3( 0.5f,  0.5f, 0.0f), glm::vec2(1.0f, 1.0f) },
            { glm::vec3(-0.5f,  0.5f, 0.0f), glm::vec2(0.0f, 1.0f) }
        };
        static uint32_t builtin_quad_index_data[] = { 0, 2, 3, 0, 1, 2 };

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

        static const MeshData BUILTIN_QUAD = {
            .vertex_data = builtin_quad_vertex_data,
            .vertex_count = sizeof(builtin_quad_vertex_data) / sizeof(VertexDataUnlit),
            .vertex_data_size = sizeof(VertexDataUnlit),
            .index_data = builtin_quad_index_data,
            .index_count = sizeof(builtin_quad_index_data) / sizeof(uint32_t)
        };
    }

    uint32_t num_mesh_assets = 0;
    uint32_t num_texture_assets = 0;
    uint32_t num_material_assets = 0;
    uint32_t num_model_assets = 0;

    std::map<IdAssetMesh, MeshData> mesh_data;
    std::map<IdAssetTexture, TextureData> texture_data;
    std::map<IdAssetMaterial, MaterialData> material_data;
    std::map<IdAssetModel, ModelData> model_data;

    void asset_manager_init() {
        // create assets on CPU
        create_mesh(BuiltinPrimitives::IDX_DEBUG_CUBE,     BuiltinPrimitives::DEBUG_CUBE_MESH_DATA);
        create_mesh(BuiltinPrimitives::IDX_DEBUG_RAY,      BuiltinPrimitives::DEBUG_RAY_MESH_DATA);
        create_mesh(BuiltinPrimitives::IDX_FULLSCREEN_TRI, BuiltinPrimitives::BUILTIN_FULLSCREEN_TRI);
        create_mesh(BuiltinPrimitives::IDX_QUAD,           BuiltinPrimitives::BUILTIN_QUAD);

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

    ModelData& get_model_data(IdAssetModel id) {
        return model_data[id];
    }

    void debug_print_material_info(const aiMaterial& ai_material_data) {
        // TMP print material properties
        CC_LOG(IMPORTANT, "%-28s\ttype\tsemantic\tsize\tvalue", "name");
        for (int j = 0; j < ai_material_data.mNumProperties; ++j) {
            const aiMaterialProperty& ai_mat_prop_data = *ai_material_data.mProperties[j];

            char buf[64];
            int buffer_offset = 0;
            aiString path;
            if (ai_mat_prop_data.mDataLength < 63)
            {
                switch(ai_mat_prop_data.mType) {
                case aiPTI_Float:
                    for(unsigned int p = 0; p < ai_mat_prop_data.mDataLength; p += sizeof(float))
                        buffer_offset += sprintf(buf + buffer_offset, "%f   ", (float) *ai_mat_prop_data.mData + p); break;
                case aiPTI_Double:
                    for(unsigned int p = 0; p < ai_mat_prop_data.mDataLength; p += sizeof(double))
                        buffer_offset += sprintf(buf + buffer_offset, "%f   ", (double) *ai_mat_prop_data.mData + p); break;
                case aiPTI_Integer:
                    for(unsigned int p = 0; p < ai_mat_prop_data.mDataLength; p += sizeof(int))
                        buffer_offset += sprintf(buf + buffer_offset, "%d   ", (int)  *ai_mat_prop_data.mData + p); break;
                case aiPTI_String:
                    if (ai_mat_prop_data.mSemantic != aiTextureType_NONE && ai_mat_prop_data.mSemantic != aiTextureType_UNKNOWN) {
                        ai_material_data.GetTexture(static_cast<aiTextureType>(ai_mat_prop_data.mSemantic), 0, &path);
                        memcpy(buf, path.C_Str(), path.length + 1); // we already ensured that the string is short enough
                    }
                    else
                    {
                        //buffer_offset = sprintf(buf, "%s", ai_mat_prop_data.mData);
                        memcpy(buf, ai_mat_prop_data.mData, ai_mat_prop_data.mDataLength);
                        // some string are prefixe with garbage. temporary solution: remove all terminators
                        for(int i = 0; i < ai_mat_prop_data.mDataLength - 1; ++i)
                            if(buf[i] == 0)
                                buf[i] = '_';
                    }
                    break;
                //case aiPTI_Buffer:  sprintf(buf, "%s", ai_mat_prop_data.mData); break;
                case aiPTI_Buffer:  memcpy(buf, ai_mat_prop_data.mData, ai_mat_prop_data.mDataLength); break;

                default:            sprintf(buf, "<unrecognized buffer type>"); break;
                }
            }
            else
                sprintf(buf, "<too much data, check in debug modw>");

            CC_LOG(VERBOSE, "%-28s\t%d\t%d\t\t%d\t%s", ai_mat_prop_data.mKey.C_Str(), ai_mat_prop_data.mType, ai_mat_prop_data.mSemantic, ai_mat_prop_data.mDataLength, buf);
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
        IdAssetTexture TMP_tex_environment_id
    ) {
        CC_LOG(IMPORTANT, "Loading model %s...", path);
        uint32_t model_idx = num_model_assets++;
        ModelData new_model_data;

        // Read the file using Assimp importer
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_CalcTangentSpace
            | aiProcess_Triangulate
            | aiProcess_SortByPType
            | aiProcess_MakeLeftHanded
            | aiProcess_FlipWindingOrder
            | aiProcess_FlipUVs
        );

        CC_ASSERT(scene, "[assimp] could not load %s", path);
        CC_LOG(LOG, "assimp scene loaded");
        CC_LOG(LOG, "materials: %d", scene->mNumMaterials);
        CC_LOG(LOG, "meshes: %d", scene->mNumMeshes);

        // create material
        std::map<unsigned int, IdAssetMaterial> material_map;
        for(int i = 0; i < scene->mNumMaterials; ++i) {
            // load textures
            const aiMaterial& ai_material_data = *scene->mMaterials[i];

            //if (i < 3)
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

            
            IdAssetTexture tex_idx_arm;
            IdAssetTexture tex_idx_diffuse;
            IdAssetTexture tex_idx_normal;

            //// had-hoc semantics for models taken from ituGL
            //{
            //    tex_idx_arm     = load_tex(aiTextureType_SHININESS, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, TEX_FORMAT_RGB_A, BuiltinPrimitives::IDX_TEX_BLACK, base_path);
            //    tex_idx_diffuse = load_tex(aiTextureType_DIFFUSE, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, TEX_FORMAT_RGB_A, BuiltinPrimitives::IDX_TEX_WHITE, base_path);
            //    tex_idx_normal  = load_tex(aiTextureType_NORMALS, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB, TEX_FORMAT_NORM, BuiltinPrimitives::IDX_TEX_BLUE_NORM, base_path);
            //}

            // had-hoc semantics for bistrot model
            {
                tex_idx_diffuse = load_tex(aiTextureType_DIFFUSE,  ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, TEX_FORMAT_RGB_A, BuiltinPrimitives::IDX_TEX_WHITE,     base_path);
                tex_idx_arm     = load_tex(aiTextureType_SPECULAR, ai_material_data, vkc::Assets::TEX_CHANNELS_RGB_A, TEX_FORMAT_RGB_A, BuiltinPrimitives::IDX_TEX_BLACK,     base_path);
                tex_idx_normal  = load_tex(aiTextureType_NORMALS,  ai_material_data, vkc::Assets::TEX_CHANNELS_RGB,   TEX_FORMAT_NORM,  BuiltinPrimitives::IDX_TEX_BLUE_NORM, base_path);
            }

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

        print_fourcc_count();

        new_model_data.meshes_count = scene->mNumMeshes;
        new_model_data.meshes = new IdAssetMesh[scene->mNumMeshes];
        new_model_data.meshes_material = new IdAssetMesh[scene->mNumMeshes];
        for(int i = 0; i < scene->mNumMeshes; ++i) {
            IdAssetMesh mesh_idx = num_mesh_assets++;
            MeshData new_submesh_data { };
            const aiMesh& ai_mesh_data = *scene->mMeshes[i];

            uint64_t stride_uv = sizeof(*ai_mesh_data.mTextureCoords[0]);

            new_submesh_data.vertex_data_size = sizeof(VertexData);
            new_submesh_data.vertex_data = new VertexData[ai_mesh_data.mNumVertices];
            new_submesh_data.vertex_count = ai_mesh_data.mNumVertices;
            VertexData* p = (VertexData*)new_submesh_data.vertex_data;
            for(int j = 0; j < ai_mesh_data.mNumVertices; ++j) {
                VertexData vertex_data = { };
                p[j].position.x = ai_mesh_data.mVertices[j].x;
                p[j].position.y = -ai_mesh_data.mVertices[j].z;
                p[j].position.z = ai_mesh_data.mVertices[j].y;

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
            new_submesh_data.index_data = (uint32_t*)malloc(sizeof(uint32_t) * num_indices);
            new_submesh_data.index_count = num_indices;
            uint32_t idx = 0;
            for(int j = 0; j < ai_mesh_data.mNumFaces; ++j) {
                new_submesh_data.index_data[idx + 0] = ai_mesh_data.mFaces[j].mIndices[0];
                new_submesh_data.index_data[idx + 1] = ai_mesh_data.mFaces[j].mIndices[1];
                new_submesh_data.index_data[idx + 2] = ai_mesh_data.mFaces[j].mIndices[2];

                idx += 3;
            }

           mesh_data[mesh_idx] = new_submesh_data;
           new_model_data.meshes[i] = mesh_idx;
           new_model_data.meshes_material[i] = material_map[ai_mesh_data.mMaterialIndex];
           CC_LOG(VERBOSE, "loaded mesh %d/%d", i+1, scene->mNumMeshes);
        }

        model_data[model_idx] = new_model_data;
        return model_idx;
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
            num_material_assets,
            num_model_assets
        };

        fwrite(sizes, sizeof(uint32_t), 4, fp);

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
            fwrite(&data,            sizeof(TextureData) - sizeof(std::vector<unsigned char>), 1, fp);
            // annoying, we have to manually write size if we use std
            size_t num_bytes = data.data.size();
            fwrite(&num_bytes, sizeof(size_t), 1, fp);
            fwrite(data.data.data(), sizeof(unsigned char),  data.data.size(), fp);
        }

        for(auto& kp : material_data) {
            IdAssetMaterial id = kp.first;
            MaterialData& data = kp.second;
            fwrite(&id,                     sizeof(IdAssetMaterial), 1,                       fp);
            fwrite(&data,                   sizeof(MaterialData) - sizeof(std::vector<IdAssetTexture>), 1, fp);
            // annoying, we have to manually write size if we use std::vector
            size_t num_views = data.image_views.size();
            fwrite(&num_views, sizeof(size_t), 1, fp);
            fwrite(data.image_views.data(), sizeof(IdAssetTexture),  data.image_views.size(), fp);
        }

        for(auto& kp : model_data) {
            IdAssetModel id = kp.first;
            ModelData& data = kp.second;
            fwrite(&id,                  sizeof(IdAssetModel),    1,                 fp);
            fwrite(&data,                sizeof(ModelData),       1,                 fp);
            fwrite(data.meshes,          sizeof(IdAssetMesh),     data.meshes_count, fp);
            fwrite(data.meshes_material, sizeof(IdAssetMaterial), data.meshes_count, fp);
        }

        fclose(fp);
        CC_LOG_SYS_ERROR();
        //CC_ASSERT(fclose(fp) == 0, "error closing file");
    }
    void asset_db_load(const char *path) {
        FILE* fp = fopen(path, "rb+");

        CC_ASSERT(fp, "error opening file");

        uint32_t sizes[4];

        fread(sizes, sizeof(uint32_t), 4, fp);
        num_mesh_assets     = sizes[0];
        num_texture_assets  = sizes[1];
        num_material_assets = sizes[2];
        num_model_assets    = sizes[3];

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

        for(int i = 0; i < num_model_assets; ++i) {
            IdAssetModel id;
            ModelData data;
            fread(&id,                  sizeof(IdAssetModel),    1,                 fp);
            fread(&data,                sizeof(ModelData),       1,                 fp);
            data.meshes          = new IdAssetMesh[data.meshes_count];
            data.meshes_material = new IdAssetMaterial[data.meshes_count];
            fread(data.meshes,          sizeof(IdAssetMesh),     data.meshes_count, fp);
            fread(data.meshes_material, sizeof(IdAssetMaterial), data.meshes_count, fp);

            model_data[id] = data;
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
        int mips = 1;

        void* pixels;
        bool use_stbi = std::filesystem::path(path).extension() != ".dds";

        uint32_t size;
        if (use_stbi) {
            stbi_set_flip_vertically_on_load(flip_vertical ? 1 : 0);
            pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, requested_channels_count);
            size = texWidth * texHeight * requested_channels_count;
        }
        else
        {
            FILE* fp = fopen(path, "rb");
            fseek(fp, 0, SEEK_END);
            size_t file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            unsigned char * file_buf = (unsigned char *)malloc(file_size);
            fread(file_buf, 1, file_size, fp);
            fclose(fp);

            int file_format;

            pixels = rl_load_dds_from_memory(
                file_buf,
                file_size,
                &texWidth,
                &texHeight,
                &file_format,
                &mips,
                &size
            );


            // overrides format since file carries it in header
            format = static_cast<TexFormat>(file_format);

            free(file_buf);
        }

        if (pixels == nullptr) {
            CC_LOG(WARNING, "missing texture at path %s", path);
            return IDX_MISSING_TEXTURE;
        }



        TextureData data;
        data.viewType = viewType;
        data.width = (uint16_t)texWidth;
        data.height = (uint16_t)texHeight;
        data.channelsCount = (uint8_t)requested_channels_count;
        data.mipmaps = mips;
        data.channels = channels;
        data.format = format;
        data.data.resize(size);
        memcpy(data.data.data(), pixels, size);

        if (use_stbi)
            stbi_image_free(pixels);
        else
            free(pixels);

        if (viewType == TEX_VIEW_TYPE_CUBE) {
            data.width /= 4;
            data.height /= 3;
        }
        texture_data[id] = data;
        return id;
    }
}