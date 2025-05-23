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

#include <AssetManager.hpp>

#include <glm/glm.hpp>

// =================================================================
// base forward pass
// =================================================================

enum DebugLightComponents : uint32_t {
    DIRECT   = 0b001,
    INDIRECT = 0b010,
    AMBIENT  = 0b100,
};

// TODO where to pit this? Together with vertexData?
struct DataUniformFrame {
    // camera
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cam_pos;
    uint32_t padding_0;

    // lighting
    glm::vec3 light_ambient;
    uint32_t padding_1;

    glm::vec3 light_dir;

    uint32_t DEBUG_light_components;

    glm::vec3 light_color;
    float light_intensity;

    // application
    uint32_t frame;
};

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
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexData, tangent)
    },
    (VkVertexInputAttributeDescription) {
        .location = 4,
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


// =================================================================
// VFX
// =================================================================

// FIXME create descriptors based on what we need. Either:
//    - create them on the fly
//    - specify few meaninful sets, WITH PROPER NAMES
inline const VkVertexInputAttributeDescription* vertexDataFX_getAttributeDescriptions() {
    static VkVertexInputAttributeDescription ret = (VkVertexInputAttributeDescription){
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
    };
    return &ret;
}

// =================================================================
// Debug
// =================================================================

typedef struct {
    glm::mat4 viewProj;
} DataUniformFrameDebug;

typedef struct {
    glm::mat4 model;
    glm::vec3 color;
} DataUniformModelDebug;

// FIXME create descriptors based on what we need. Either:
//    - create them on the fly
//    - specify few meaninful sets, WITH PROPER NAMES
inline const VkVertexInputBindingDescription* vertexDebug_getBindingDescriptions() {
    static VkVertexInputBindingDescription ret = (VkVertexInputBindingDescription){
        .binding = 0,
        .stride = sizeof(glm::vec3),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    return &ret;
}

inline const VkVertexInputAttributeDescription* vertexDebug_getAttributeDescriptions() {
    static VkVertexInputAttributeDescription ret = (VkVertexInputAttributeDescription){
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(DataUniformFrameDebug, viewProj)
    };
    return &ret;
}

// =================================================================
// Trail
// =================================================================
struct DataUniformTrail {
    float radius;
    int offset_dir;
};


// =================================================================
// Skybox
// =================================================================
typedef struct {
    glm::vec3 position;
} VertexDataSkybox;

static const VkVertexInputAttributeDescription attributeDescriptions_skybox[] = {
    (VkVertexInputAttributeDescription) {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexDataSkybox, position),
    }
};

static const VkVertexInputBindingDescription bindingDescriptions_skybox[] = {
    (VkVertexInputBindingDescription) {
        .binding = 0,
        .stride = sizeof(VertexDataSkybox),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    }
};
static const uint32_t bindingDescriptionsCount_skybox = sizeof(bindingDescriptions_skybox) / sizeof(VkVertexInputBindingDescription);

static const uint32_t attributeDescriptions_skyboxCount = sizeof(attributeDescriptions_skybox) / sizeof(VkVertexInputAttributeDescription);

inline const VkVertexInputBindingDescription* vertexData_getBindingDescriptions_Skybox() {
    return bindingDescriptions_skybox;
}

inline const uint32_t vertexData_getBindingDescriptionsCount_Skybox() {
    return bindingDescriptionsCount_skybox;
}

inline const VkVertexInputAttributeDescription* vertexData_getAttributeDescriptions_Skybox() {
    return attributeDescriptions_skybox;
}

inline const uint32_t vertexData_getAttributeDescriptions_SkyboxCount() {
    return attributeDescriptions_skyboxCount;
}

// =================================================================
// Unit
// =================================================================

static const VkVertexInputAttributeDescription attributeDescriptions_unlit[] = {
    (VkVertexInputAttributeDescription) {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexDataUnlit, position),
    },
    (VkVertexInputAttributeDescription) {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(VertexDataUnlit, texCoords),
    }
};

static const VkVertexInputBindingDescription bindingDescriptions_unlit[] = {
    (VkVertexInputBindingDescription) {
        .binding = 0,
        .stride = sizeof(VertexDataUnlit),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
}
};
static const uint32_t bindingDescriptionsCount_unlit = sizeof(bindingDescriptions_unlit) / sizeof(VkVertexInputBindingDescription);

static const uint32_t attributeDescriptions_unlitCount = sizeof(attributeDescriptions_unlit) / sizeof(VkVertexInputAttributeDescription);

inline const VkVertexInputBindingDescription* vertexData_getBindingDescriptions_Unlit() {
    return bindingDescriptions_unlit;
}

inline const uint32_t vertexData_getBindingDescriptionsCount_Unlit() {
    return bindingDescriptionsCount_unlit;
}

inline const VkVertexInputAttributeDescription* vertexData_getAttributeDescriptions_Unlit() {
    return attributeDescriptions_unlit;
}

inline const uint32_t vertexData_getAttributeDescriptions_UnlitCount() {
    return attributeDescriptions_unlitCount;
}