#include <VKRenderer.hpp>

#include <core/DrawCall.hpp>
#include <assets/AssetManager.hpp>

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
    glm::vec3 camera_pos;
    glm::vec3 camera_rot;

    DataUniformMaterial tmp_data_uniform_material = (DataUniformMaterial) {
        .ambient = 1,
        .diffuse = 1,
        .specular = 1,
        .specular_exp = 200,
    };

    const uint32_t drawcall_cout = 1;
    std::vector<DataUniformModel> model_data;

    std::vector<std::string> TMP_mesh_names;
    std::vector<vkc::Assets::IdAssetMesh> TMP_mesh_idxs;
    int TMP_object_curr_idx = 0;

    vkc::Assets::IdAssetMaterial idMaterialModels;

    // trail
    const int trail_size = 64;
    std::vector<glm::vec3> trail_positions(trail_size);
    vkc::Assets::IdAssetMaterial idMaterialTrail;
    DataUniformTrail trail_data;
    uint32_t trail_mesh_id;

    VertexData vertex_data_trail[TMP_Update::trail_size];
    uint32_t indices[TMP_Update::trail_size];;
    vkc::Assets::MeshData trail_mesh;

    void TMP_update_gui(vkc::Rect2DI window_size, DataUniformFrame& ubo) {
        ImGui::Begin("tmp_update_info");
        ImGui::SeparatorText("Object data");
        const char* current_item = TMP_mesh_names[TMP_object_curr_idx].c_str();

        if (ImGui::BeginCombo("##combo", current_item)) // The second parameter is the label previewed before opening the combo.
        {
            for (int n = 0; n < TMP_mesh_names.size(); n++)
            {
                const char* entry_item = TMP_mesh_names[n].c_str();
                bool is_selected = (current_item == entry_item); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(TMP_mesh_names[n].c_str(), is_selected))
                {
                    current_item = entry_item;
                    TMP_object_curr_idx = n;
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }

        ImGui::SeparatorText("Frame data");
        ImGui::DragFloat3("Light Color Ambient", &ubo.light_ambient.x);
        ImGui::DragFloat3("Light Color Light", &ubo.light_color.x);
        ImGui::DragFloat3("Light Direction", &ubo.light_dir.x);
        ImGui::DragFloat("Light Intensity", &ubo.light_intensity);

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

        if (ImGui::IsKeyDown(ImGuiKey_W)) { local_camera_pos.z -= SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_S)) { local_camera_pos.z += SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_A)) { local_camera_pos.x -= SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_D)) { local_camera_pos.x += SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) { local_camera_pos.y -= SPEED_MOV; dirty = true; }
        if (ImGui::IsKeyDown(ImGuiKey_E)) { local_camera_pos.y += SPEED_MOV; dirty = true; }

        if (dirty)
        {
            glm::vec3 rot = glm::radians(camera_rot);
            glm::mat4 rot_matrix = glm::identity<glm::mat4>();

            const glm::vec3 FORWARD = glm::vec3(0, 0, -1);
            const glm::vec3 UP = glm::vec3(0, 1, 0);
            rot_matrix = glm::rotate(rot_matrix, rot.y, glm::vec3(0, 1, 0));
            rot_matrix = glm::rotate(rot_matrix, rot.x, glm::vec3(1, 0, 0));
            rot_matrix = glm::rotate(rot_matrix, rot.z, glm::vec3(0, 0, 1));

            glm::vec3 global_camera_pos = glm::vec3((rot_matrix)*glm::vec4(local_camera_pos, 1.0f));

            camera_pos += global_camera_pos;
            glm::mat4 translate_matrix = glm::translate(glm::identity<glm::mat4>(), camera_pos);
            camera_world = translate_matrix * rot_matrix;
            camera_view = glm::inverse(camera_world);
        }
        ImGui::End();
    }

    void updateUniformBuffer(vkc::Rect2DI window_size, DataUniformFrame& ubo) {
        // zoom out depending on loaded objects
        float l = glm::sqrt(drawcall_cout);
        //float l = 2;
        float fow = (float)window_size.width / window_size.height;
        camera_proj = glm::perspective(glm::radians(45.0f), fow, 0.1f, 100.0f);
        //perspective_projection = glm::perspective(glm::radians(45.0f), fow, 0.1f, 10.0f);

        ubo.view = camera_view;
        ubo.proj = camera_proj;

        // invert up axis
        ubo.proj[1][1] *= -1;
    }
}


class TestRenderer : public VKRenderer {
    void init() override {
        // load mesh and textures
        TMP_Update::TMP_mesh_idxs = vkc::Assets::load_meshes_from_folder("res/models/pack_prototype");
        auto TMP_texture_idx = vkc::Assets::load_texture("res/textures/colormap.png", vkc::Assets::TEX_CHANNELS_RGB_A);

        // create uniform data for each model
        TMP_Update::model_data.resize(TMP_Update::drawcall_cout);
        int l = glm::sqrt(TMP_Update::drawcall_cout);
        for (int i = 0; i < TMP_Update::drawcall_cout; ++i)
            TMP_Update::model_data[i] = DataUniformModel{ .model = glm::translate(glm::mat4(1.0f), glm::vec3(i % l - l / 2, 0, i / l - l / 2)) };

        // set mesh names for GUI
        TMP_Update::TMP_mesh_names.resize(TMP_Update::TMP_mesh_idxs.size());
        for (int i = 0; i < TMP_Update::TMP_mesh_idxs.size(); ++i)
            TMP_Update::TMP_mesh_names[i] = std::to_string(TMP_Update::TMP_mesh_idxs[i]);

        TMP_Update::idMaterialModels = vkc::Assets::create_material(vkc::Assets::MaterialData{
            .id_render_pass = 0,
            .id_pipeline = 0,
            .uniform_data_material = &TMP_Update::tmp_data_uniform_material
         });

        TMP_Update::idMaterialTrail = vkc::Assets::create_material(vkc::Assets::MaterialData{
            .id_render_pass = 0,
            .id_pipeline = 1,
            // hack, material data of the trail is smaller than the one for materials, but the base matches
            .uniform_data_material = &TMP_Update::tmp_data_uniform_material
        });

        // create trial geo

        for (int i = 0; i < TMP_Update::trail_size; ++i) {
            TMP_Update::vertex_data_trail[i].position = glm::vec3(0.0f);
            TMP_Update::vertex_data_trail[i].color = glm::vec3(1.0f);
            TMP_Update::vertex_data_trail[i].normal = glm::vec3(0.0f);
            TMP_Update::vertex_data_trail[i].texCoords = glm::vec2(i * 1.0f / 10.0f, 0.0f);
            TMP_Update::indices[i] = i;
        }
        TMP_Update::trail_mesh.vertex_count = TMP_Update::trail_size;
        TMP_Update::trail_mesh.vertex_data = TMP_Update::vertex_data_trail;
        TMP_Update::trail_mesh.index_count = TMP_Update::trail_size;
        TMP_Update::trail_mesh.index_data = TMP_Update::indices;
        TMP_Update::trail_mesh.vertex_data_size = sizeof(TMP_Update::vertex_data_trail[0]);

        TMP_Update::trail_mesh_id = vkc::Assets::create_mesh(TMP_Update::trail_mesh);
        /*TMP_Update::trail_mesh_id = vkc::Drawcall::createModelBuffers(
            (void*)TMP_Update::vertex_data_trail,
            (uint32_t)TMP_Update::trail_size * sizeof(TMP_Update::vertex_data_trail[0]),
            (uint32_t*)TMP_Update::indices,
            (uint32_t)TMP_Update::trail_size * sizeof(TMP_Update::indices[0]),
            get_device_handle(),
            get_render_context_obj()
        );
        vkc::Drawcall::add_debug_name(
            TMP_Update::trail_mesh_id,
            get_device_handle(),
            get_instance_obj(),
            "trail_buffers"
        );*/
    }

    void update() override {
        TMP_Update::updateUniformBuffer(
            get_window_size(),
            get_ubo_reference()
        );

        // model
        const glm::vec3 UP(0.0f, 1.0f, 0.0f);
        const float char_speed_mov = 3.0f;
        const float char_speed_rot = 3.0f;
        auto& io = ImGui::GetIO();
        glm::vec3 char_mov(0.0f);
        float char_rot = 0;
        if (ImGui::IsKeyDown(ImGuiKey_UpArrow))    char_mov.z += 1.0f;
        if (ImGui::IsKeyDown(ImGuiKey_DownArrow))  char_mov.z -= 1.0f;
        if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) char_rot -= 1.0f;
        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))  char_rot += 1.0f;

        float delta = get_delta_time();

        auto mtx_rot = glm::eulerAngleXYZ(0.0f, char_rot * char_speed_rot * delta, 0.0f);
        auto mtx_mov = glm::translate(TMP_Utils::global_to_local_dir(char_mov * char_speed_mov * delta, TMP_Update::model_data[0].model));

        TMP_Update::model_data[0].model = mtx_mov * TMP_Update::model_data[0].model * mtx_rot;

        // trail
        if (get_current_frame() % 2 != 0)
            return;
        for (int i = TMP_Update::trail_size - 1; i > 0; --i)
        {
            TMP_Update::vertex_data_trail[i].position = TMP_Update::vertex_data_trail[i - 1].position;
        }
        TMP_Update::vertex_data_trail[0].position = TMP_Update::model_data[0].model[3];
        TMP_Update::trail_data.radius = 0.1f;
        TMP_Update::trail_data.viewport_half_width = get_window_size().width / 2.0f;
        TMP_Update::trail_data.viewport_half_height = get_window_size().height / 2.0f;

        get_render_context_obj()->update_mesh_vertex_data(TMP_Update::trail_mesh_id, TMP_Update::vertex_data_trail, sizeof(TMP_Update::vertex_data_trail));
    }

    void render() override {
        // model
        drawcall_add(
            TMP_Update::TMP_mesh_idxs[TMP_Update::TMP_object_curr_idx],
            TMP_Update::idMaterialModels,
            &TMP_Update::model_data[0],
            sizeof(TMP_Update::model_data[0])
        );

        // trail
        drawcall_add(
            TMP_Update::trail_mesh_id,
            TMP_Update::idMaterialTrail,
            &TMP_Update::trail_data,
            sizeof(TMP_Update::trail_data)
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