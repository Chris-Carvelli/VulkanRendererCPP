#include <VKRenderer.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <cstdlib>
#include <string.h>

#include <cmath>

// global
const int GLOBAL_ZOOM = 2;

// tiles
const char* TILE_ID_FORMAT = "tile_%d_%d";
const char* TEX_ID_FORMAT  = "tex_%d_%d";
const ImGuiWindowFlags TILE_FLAGS =
    ImGuiWindowFlags_NoResize   |
    ImGuiWindowFlags_NoMove     |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoCollapse;
const int NUM_TILE_W = 3;
const int NUM_TILE_H = 3;
const ImVec2 TILE_SIZE = ImVec2(256, 256);
const ImVec4 TILE_COLOR[] = {
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
    ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
    ImVec4(0.0f, 0.0f, 1.0f, 1.0f)
};

// player
float player_speed = 3;
float player_pos_x = 0;
float player_pos_y = 0;

// map
const char* MAP_ID = "map_overlay";
const ImGuiWindowFlags MAP_FLAGS =
    ImGuiWindowFlags_NoResize     |
    ImGuiWindowFlags_NoMove       |
    ImGuiWindowFlags_NoCollapse   |
    ImGuiWindowFlags_NoTitleBar;
const ImVec2 MAP_SIZE = ImVec2(400, 400);
ImVec2 map_pos = ImVec2(400, 300);


const char* DEBUG_ID = "debug";
const ImGuiWindowFlags DEBUG_FLAGS = ImGuiWindowFlags_None;
int tmp_debug_int_0;
int tmp_debug_int_1;


bool is_column_visible(float pox_x);

class TestRenderer : public VKRenderer {
    void init() override {

    }

    void update() override {
        // move player based on world/cartesian coordinates
        // x: positive right
        // y: positive up
        if (ImGui::IsKeyDown(ImGuiKey_W)) player_pos_y += player_speed;
        if (ImGui::IsKeyDown(ImGuiKey_S)) player_pos_y -= player_speed;
        if (ImGui::IsKeyDown(ImGuiKey_A)) player_pos_x -= player_speed;
        if (ImGui::IsKeyDown(ImGuiKey_D)) player_pos_x += player_speed;

        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) player_pos_y += 1;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))
            player_pos_y -= 1;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
            player_pos_x -= 1;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
            player_pos_x += 1;
        // pixel precise movement
    }

    void render() override {
    }

    void gui() override {
        // tiles
        for(int i = 0; i < NUM_TILE_W; ++i)
            for (int j = 0; j < NUM_TILE_H; ++j) {
                // imgui only stuff
                char tile_id[9];
                sprintf(tile_id, TILE_ID_FORMAT, i, j);

                /// POSITION

                ImVec2 pos_world = ImVec2(0, 0);


                // account for player position
                pos_world.x += -player_pos_x; // if player moves right, map needs to scroll left
                pos_world.y +=  player_pos_y; // world y axis is already flipped compared to window
                                              // coords, double negation becomes positive


                ImVec2 pos_tile_grid = ImVec2(
                    TILE_SIZE.x * (i),
                    TILE_SIZE.y * (j)
                );

                // reorder tiles
                ImVec2 pos_tile_world;
                pos_tile_world.x = (int)(pos_world.x) % (int)TILE_SIZE.x;
                pos_tile_world.y = (int)(pos_world.y) % (int)TILE_SIZE.y;

                // combine grid and world position
                ImVec2 pos;
                ImVec2 offset;
                offset.x = 0;
                offset.y = 0;
                /*offset.x = std::copysign(TILE_SIZE.x / 2, pos_tile_world.x);
                offset.y = std::copysign(TILE_SIZE.y / 2, pos_tile_world.y);*/
                pos = pos_tile_world + pos_tile_grid - offset;

                // center with minimap overlay
                // .1 move tiles at minimap position
                pos += map_pos;
                // .2 center with minimap
                //pos -= MAP_SIZE * 0.5f;

                /*if (!is_column_visible(pos.x))
                    continue;*/

                // load texture
                char tex_id[9];
                sprintf(tex_id, TEX_ID_FORMAT, i, j);
                int tex_file_id[2];
                int tex_id_absolute_x = (int)(pos_world.x + TILE_SIZE.x) / (int)TILE_SIZE.x;
                int tex_id_absolute_y = (int)(pos_world.y + TILE_SIZE.y) / (int)TILE_SIZE.y;
                int tex_id_mod_x = tex_id_absolute_x % NUM_TILE_W;
                int tex_id_mod_y = tex_id_absolute_y % NUM_TILE_H;
                tex_file_id[0] = tex_id_absolute_x -i;
                tex_file_id[1] = tex_id_absolute_y -j;

                // render
                //ImGui::PushStyleColor(ImGuiCol_WindowBg, TILE_COLOR[tex_file_id[0] % NUM_TILE_W]); // Set window background to red
                ImGui::Begin(tile_id, NULL, TILE_FLAGS);
                ImGui::Text(tile_id);
                ImGui::DragInt2(tex_id, tex_file_id, 0, 0, 0, "%d", ImGuiSliderFlags_NoInput);
                ImGui::SetWindowSize(tile_id, TILE_SIZE / GLOBAL_ZOOM);
                ImGui::SetWindowPos(tile_id, pos / GLOBAL_ZOOM);
                //ImGui::PopStyleColor();
                ImGui::End();
            }

        // map overlay
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin(MAP_ID, NULL, MAP_FLAGS);
        ImGui::SetWindowSize(MAP_ID, MAP_SIZE / GLOBAL_ZOOM);
        ImGui::SetWindowPos(MAP_ID, map_pos / GLOBAL_ZOOM);
        ImGui::End();

        // debug
        ImGui::Begin(DEBUG_ID, NULL, DEBUG_FLAGS);

        ImGui::SeparatorText("Player");
        ImGui::DragFloat("Speed", &player_speed, -10, 10, 0.25f, "%0.2f");
        ImGui::DragFloat("X", &player_pos_x, -3000, 3000, 0.0f, "%0.3f");
        ImGui::DragFloat("Y", &player_pos_y, -3000, 3000, 0.0f, "%0.3f");

        ImGui::SeparatorText("Calculations");
        ImGui::SetNextItemWidth(64);
        ImGui::LabelText("Tile modulo X", "%3d", tmp_debug_int_0);
        ImGui::SetNextItemWidth(64);
        ImGui::LabelText("Tile modulo Y", "%3d", tmp_debug_int_1);

        ImGui::SeparatorText("Controls");
        ImGui::Text("WASD: movement");
        ImGui::Text("Arrows: movement\n(pixel/unit precise)");

        ImGui::End();
    }
};

bool is_column_visible(float pos_x) {
    if (pos_x + TILE_SIZE.x < map_pos.x)
        return false;

    if (pos_x > map_pos.x + MAP_SIZE.x)
        return false;

    return true;
}

int main() {
    TestRenderer app;

    app.run();

    return 0;
}