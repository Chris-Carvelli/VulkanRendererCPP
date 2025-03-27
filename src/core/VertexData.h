#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

typedef struct {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoords;
} VertexData;

typedef struct {
    glm::mat4 model_matrix;
} InstanceData;

static const VkVertexInputBindingDescription bindingDescriptions[] = {
    (VkVertexInputBindingDescription) {
        .binding = 0,
        .stride = sizeof(VertexData),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    }
};
static const uint32_t bindingDescriptionsCount = sizeof(bindingDescriptions) / sizeof(VkVertexInputBindingDescription);

static const VkVertexInputAttributeDescription attributeDescriptions[] = {
    (VkVertexInputAttributeDescription) {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexData, position),
    },
    (VkVertexInputAttributeDescription) {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexData, color)
    },
    (VkVertexInputAttributeDescription) {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(VertexData, texCoords)
    }
};
static const uint32_t attributeDescriptionsCount = sizeof(attributeDescriptions) / sizeof(VkVertexInputAttributeDescription);

// VertexData
const VkVertexInputBindingDescription* vertexData_getBindingDescriptions() {
    return bindingDescriptions;
}

const uint32_t vertexData_getBindingDescriptionsCount() {
    return bindingDescriptionsCount;
}

const VkVertexInputAttributeDescription* vertexData_getAttributeDescriptions() {
    return attributeDescriptions;
}

const uint32_t vertexData_getAttributeDescriptionsCount() {
    return attributeDescriptionsCount;
}