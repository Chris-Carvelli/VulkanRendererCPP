#include <AssetManager.hpp>

#define TMP_PIPELINE_CONFIG_SKYBOX 0
#define TMP_PIPELINE_CONFIG_PBR    1
#define TMP_PIPELINE_CONFIG_UNLIT  2

const char* test_texture_paths[] {
    "res/models/Bistro_v5_2/Textures/curtainA_BaseColor.dds",
    "res/models/Bistro_v5_2/Textures/curtainA_Specular.dds",
    "res/models/Bistro_v5_2/Textures/curtainA_Normal.dds"
};

int main() {

    // skybox
    //auto TMP_tex_idx_skybox = vkc::Assets::load_texture("res/textures/default_cubemap.png", vkc::Assets::TEX_CHANNELS_RGB_A, vkc::Assets::TEX_VIEW_TYPE_CUBE);
    auto TMP_tex_idx_skybox = vkc::Assets::load_texture(
        "res/models/Bistro_v5_2/san_giuseppe_bridge_4k.hdr",
        vkc::Assets::TEX_VIEW_TYPE_2D,
        //VK_FORMAT_R8G8B8_SRGB,
        VK_FORMAT_R32G32B32_SFLOAT,
        true,
        true
    );

    auto mat = vkc::Assets::MaterialData{
        .id_pipeline_config = TMP_PIPELINE_CONFIG_SKYBOX,
        .id_render_pass = 0,
        .id_pipeline = 1,
        .uniform_data_material = nullptr,
        .image_views = { TMP_tex_idx_skybox }
    };
    vkc::Assets::create_material(mat);


    //// texture viewing
    //for(auto path : test_texture_paths)
    //{
    //    auto tex_id = vkc::Assets::load_texture(path);
    //    auto mat = vkc::Assets::MaterialData{
    //        .id_pipeline_config = TMP_PIPELINE_CONFIG_UNLIT,
    //        .id_render_pass = 0,
    //        .uniform_data_material = nullptr,
    //        .image_views = { tex_id }
    //    };
    //    auto mat_id = vkc::Assets::create_material(mat);
    //    CC_LOG(IMPORTANT, "%d", mat_id);
    //}



    // load mesh and textures
    std::vector<vkc::Assets::IdAssetMesh> meshes; 
    std::vector<vkc::Assets::IdAssetMaterial> materials;
    vkc::Assets::load_model(
        "res/models/Bistro_v5_2/BistroInterior.fbx",
        "res/models/Bistro_v5_2/",
        /*"res/models/camera/camera.obj",
        "res/models/camera/",*/
        TMP_tex_idx_skybox
        //-1
    );

     vkc::Assets::asset_db_dump("res/asset_db.bin");

    return 0;
}