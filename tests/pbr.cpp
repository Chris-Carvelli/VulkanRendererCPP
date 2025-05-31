#include <VKRenderer.hpp>

#include <core/DrawCall.hpp>
#include <AssetManager.hpp>

// TMP_Update includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <cstdlib>
#include <string>

namespace TMP_Utils {
    inline glm::vec3 global_to_local_dir(glm::vec3 global, glm::mat4 m) {
        auto ret = glm::vec3(m * glm::vec4(global, 0.0f));
        return ret;
    }

    inline glm::vec3 global_to_local_point(glm::vec3 global, glm::mat4 m) {
        return glm::vec3(m * glm::vec4(global, 1.0f));
    }

    inline glm::vec3 local_to_global_dir(glm::vec3 global, glm::mat4 m) {
        auto ret = glm::vec3(glm::inverse(m) * glm::vec4(global, 0.0f));
        printf("%.3f   %.3f   %.3f\n", ret.x, ret.y, ret.z);
        return ret;
    }
}

namespace TMP_Update {
    // camera
    glm::mat4 camera_world;
    glm::mat4 camera_proj;
    glm::mat4 camera_view;
    glm::vec3 camera_pos = glm::vec3(0.3f, 2.5f, 0.5f);
    glm::vec3 camera_rot = glm::vec3(-5.0f, -106.0f, 0.0f);

    DataUniformMaterial tmp_data_uniform_material = (DataUniformMaterial) {
        .ambient = 1,
        .diffuse = 1,
        .specular = 1,
        .specular_exp = 200,
    };

    const uint32_t drawcall_cout = 3;
    std::vector<DataUniformModel> model_data;

    std::vector<vkc::Assets::IdAssetMesh> TMP_mesh_idxs;
    std::vector<vkc::Assets::IdAssetMesh> TMP_mat_idxs;

    vkc::Assets::IdAssetMaterial idMaterialSkybox;

    bool pbr_use_light_direct = true;
    bool pbr_use_light_indirect = true;
    bool pbr_use_light_ambient = false;

    void set_camera_mtx(glm::vec3 local_camera_pos) {
        glm::vec3 rot = glm::radians(camera_rot);
        glm::mat4 rot_matrix = glm::identity<glm::mat4>();

        const glm::vec3 FORWARD = glm::vec3(0, 0, -1);
        const glm::vec3 UP = glm::vec3(0, 1, 0);
        rot_matrix = glm::rotate(rot_matrix, rot.y, glm::vec3(0, 1, 0));
        rot_matrix = glm::rotate(rot_matrix, rot.x, glm::vec3(1, 0, 0));
        rot_matrix = glm::rotate(rot_matrix, rot.z, glm::vec3(0, 0, 1));

        glm::vec3 global_camera_pos = glm::vec3((rot_matrix)*glm::vec4(local_camera_pos, 1.0f));

        camera_pos += global_camera_pos;
        glm::mat4 translate_matrix = glm::translate(camera_pos);
        camera_world = translate_matrix * rot_matrix;
        camera_view = glm::inverse(camera_world);
    }

    void TMP_update_gui(vkc::Rect2DI window_size, DataUniformFrame& ubo) {
        ImGui::Begin("tmp_update_info");
        ImGui::SeparatorText("Object data");

        ImGui::SeparatorText("Frame data");
        ImGui::DragFloat3("Light Color Ambient", &ubo.light_ambient.x);
        ImGui::DragFloat3("Light Color Light", &ubo.light_color.x);
        ImGui::DragFloat3("Light Direction", &ubo.light_dir.x);
        ImGui::DragFloat("Light Intensity", &ubo.light_intensity);

        ImGui::Checkbox("Use direct light", &pbr_use_light_direct);
        ImGui::Checkbox("Use indirect light", &pbr_use_light_indirect);
        ImGui::Checkbox("Use ambient light", &pbr_use_light_ambient);


        ImGui::SeparatorText("Material Data");
        ImGui::DragFloat("Ambient", &tmp_data_uniform_material.ambient);
        ImGui::DragFloat("Diffuse", &tmp_data_uniform_material.diffuse);
        ImGui::DragFloat("Specular", &tmp_data_uniform_material.specular);
        ImGui::DragFloat("Specular Exponent", &tmp_data_uniform_material.specular_exp);

        ImGui::SeparatorText("Camera");


        bool dirty = false;
        dirty = ImGui::DragFloat3("Position", &camera_pos.x);
        dirty |= ImGui::DragFloat3("Rotation", &camera_rot.x);

        // TODO use own input system
        const float SPEED_PAN = 10.0f;
        const float SPEED_MOV = 0.1f;
        const float SPEED_ROT = 0.1f;
        const float THRESHOLD_PAN = 0.2f;
        const float THRESHOLD_ROT = 0.1f;

        glm::vec3 local_camera_pos = glm::vec3(0.0f);
        glm::vec3 local_camera_rot = glm::vec3(0.0f);

        // TODO FIXME split matrices camera view and camera world
        auto& io = ImGui::GetIO();
        if (io.MouseDown[1]) {
            if (io.KeyAlt)
            {
                if (abs(io.MouseDelta.x) > THRESHOLD_PAN) local_camera_pos.x += (SPEED_PAN * io.MouseDelta.x) / window_size.width;
                if (abs(io.MouseDelta.y) > THRESHOLD_PAN) local_camera_pos.y -= (SPEED_PAN * io.MouseDelta.y) / window_size.height;
            }
            else
            {
                if (abs(io.MouseDelta.y) > THRESHOLD_ROT) camera_rot.x += io.MouseDelta.y * SPEED_ROT;
                if (abs(io.MouseDelta.x) > THRESHOLD_ROT) camera_rot.y += io.MouseDelta.x * SPEED_ROT;
            }
            dirty = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_1)) {
            camera_pos = glm::vec3(0.0f, 10.0f, 0.0f);
            camera_rot = glm::vec3(-90.0f, 0.0f, 0.0f);
            dirty = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_2)) {
            camera_pos = glm::vec3(0.0f, 0.0f, 10.0f);
            camera_rot = glm::vec3(0.0f, 0.0f, 0.0f);
            dirty = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_3)) {
            camera_pos = glm::vec3(10.0f, 0.0f, 0.0f);
            camera_rot = glm::vec3(0.0f, 90.0f, 0.0f);
            dirty = true;
        }

        if (ImGui::IsKeyDown(ImGuiKey_W)) { local_camera_pos.z -= SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_S)) { local_camera_pos.z += SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_A)) { local_camera_pos.x -= SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_D)) { local_camera_pos.x += SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) { local_camera_pos.y -= SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_E)) { local_camera_pos.y += SPEED_MOV; dirty = true; }


        ImGui::End();

        if (dirty)
            set_camera_mtx(local_camera_pos);
    }

    void updateUniformBuffer(vkc::Rect2DI window_size, DataUniformFrame& ubo) {
        // zoom out depending on loaded objects
        float l = glm::sqrt(drawcall_cout);
        //float l = 2;
        float fow = (float)window_size.width / window_size.height;
        camera_proj = glm::perspective(glm::radians(45.0f), fow, 0.1f, 1000.0f);
        //perspective_projection = glm::perspective(glm::radians(45.0f), fow, 0.1f, 10.0f);

        ubo.cam_pos = camera_pos;
        ubo.view = camera_view;
        ubo.proj = camera_proj;

        // invert up axis
        ubo.proj[1][1] *= -1;
        ubo.DEBUG_light_components = 0;
        ubo.DEBUG_light_components |= pbr_use_light_direct   * DebugLightComponents::DIRECT;
        ubo.DEBUG_light_components |= pbr_use_light_indirect * DebugLightComponents::INDIRECT;
        ubo.DEBUG_light_components |= pbr_use_light_ambient  * DebugLightComponents::AMBIENT;
    }
}


class TestRenderer : public VKRenderer {
    void init() override {
        TMP_Update::set_camera_mtx(glm::vec3(0.0f));

        vkc::Assets::asset_db_load("res/asset_db.bin");

        // create uniform data for each model
        TMP_Update::model_data.resize(TMP_Update::drawcall_cout);
        int l = glm::sqrt(TMP_Update::drawcall_cout);
        for (int i = 0; i < TMP_Update::drawcall_cout; ++i)
            TMP_Update::model_data[i] = DataUniformModel{
                //.model = glm::translate(glm::mat4(1.0f), glm::vec3(i % l - l / 2, 0, i / l - l / 2))
                .model = glm::scale(glm::vec3(0.01f))
                //.model = glm::translate(glm::vec3(i * 2, 0.0f, 0.0f)) * glm::scale(glm::vec3(1.0f))
            };
    }

    void update() override {
        if (ImGui::IsKeyDown(ImGuiKey_R)) {
            TMP_hot_reload();
        }


        // TODO the engine should do this
        int f = get_current_frame();
        get_ubo_reference().frame = f;
        TMP_Update::updateUniformBuffer(
            get_window_size(),
            get_ubo_reference()
        );
    }

    void render() override {
        // test quads
        /*for(int i = 0; i < TMP_Update::drawcall_cout; ++i)
            drawcall_add(
            vkc::Assets::BuiltinPrimitives::IDX_QUAD,
            i,
            &TMP_Update::model_data[i],
            sizeof(TMP_Update::model_data[i])
        );*/

        // model
        auto model_data = vkc::Assets::get_model_data(0);
        for(int i = 0; i < model_data.meshes_count; ++i)
        {
            drawcall_add(
                model_data.meshes[i],
                model_data.meshes_material[i],
                &TMP_Update::model_data[0],
                sizeof(TMP_Update::model_data[0])
            );
        }

        // skybox
        drawcall_add(
            vkc::Assets::BuiltinPrimitives::IDX_FULLSCREEN_TRI,
            TMP_Update::idMaterialSkybox,
            nullptr,
            0
        );
    }

    void gui() override {
        TMP_Update::TMP_update_gui(
            get_window_size(),
            get_ubo_reference()
        );
    }
};

int main() {
	TestRenderer app;

	app.run();

	return 0;
}