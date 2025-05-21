#include <AssetManager.hpp>

int main() {
    std::vector<vkc::Assets::IdAssetMesh> loaded_meshes;
    std::vector<vkc::Assets::IdAssetMaterial> loaded_materials;

    // load tech textures
    // TODO engine should do this

    // skybox
    auto TMP_tex_idx_skybox = vkc::Assets::load_texture("res/textures/default_cubemap.png", vkc::Assets::TEX_CHANNELS_RGB_A, vkc::Assets::TEX_VIEW_TYPE_CUBE);
    auto mat = vkc::Assets::MaterialData{
        .id_pipeline_config = 0,
        .id_render_pass = 0,
        .id_pipeline = 1,
        .uniform_data_material = nullptr,
        .image_views = { TMP_tex_idx_skybox }
    };
    vkc::Assets::create_material(mat);

    // load mesh and textures
    std::vector<vkc::Assets::IdAssetMesh> meshes; 
    std::vector<vkc::Assets::IdAssetMaterial> materials;
    vkc::Assets::load_model(
        "res/models/Bistro_v5_2/BistroInterior.fbx",
        "res/models/Bistro_v5_2/",
        /*"res/models/camera/camera.obj",
        "res/models/camera/",*/
        TMP_tex_idx_skybox,
        loaded_meshes,
        loaded_materials
    );

    vkc::Assets::asset_db_dump("res/asset_db.bin");

    return 0;
}