/**
* TODO this should not be called `VertexData`
*      ATM it contains all the input info of a certain pipeline
*       - VertexData        per-vertex data
*       - InstanceData      per-instance data
*       - UniformData       per-frame data
*       - *MaterialData*    material data (not implemented yet)
*/

#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

typedef struct {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoords;
} VertexData;

// TODO where to pit this? Together with vertexData?
typedef struct {
    glm::mat4 view;
    glm::mat4 proj;

    // lighting
    glm::vec3 light_ambient;
    glm::vec3 light_dir;
    glm::vec3 light_color;
    float light_intensity;
} DataUniformFrame;

typedef struct {
    float ambient;
    float diffuse;
    float specular;
    float specular_exp;
} DataUniformMaterial;

typedef struct {
    glm::mat4 model;
} DataUniformModel;

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
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexData, normal)
    },
    (VkVertexInputAttributeDescription) {
        .location = 3,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(VertexData, texCoords)
    }
};
static const uint32_t attributeDescriptionsCount = sizeof(attributeDescriptions) / sizeof(VkVertexInputAttributeDescription);

// VertexData
inline const VkVertexInputBindingDescription* vertexData_getBindingDescriptions() {
    return bindingDescriptions;
}

inline const uint32_t vertexData_getBindingDescriptionsCount() {
    return bindingDescriptionsCount;
}

inline const VkVertexInputAttributeDescription* vertexData_getAttributeDescriptions() {
    return attributeDescriptions;
}

inline const uint32_t vertexData_getAttributeDescriptionsCount() {
    return attributeDescriptionsCount;
}