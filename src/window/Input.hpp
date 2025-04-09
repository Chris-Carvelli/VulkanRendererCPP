#pragma once

#include <glm/glm.hpp>

enum KeyCode : uint32_t {
	A,
	D,
	S,
	W,
	SPACE
};

enum MouseCode : uint32_t {
	BTN_0,
	BTN_1,
	BTN_2
};
struct InputData {
	glm::ivec2 mouse_pos;
	glm::ivec2 mouse_delta;

	MouseCode 
};