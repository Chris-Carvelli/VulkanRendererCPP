#include "Queue.hpp"

#include <core/Device.hpp>

namespace vkc {
    Queue::Queue(Device& device, uint32_t family_index, VkQueueFamilyProperties properties, VkBool32 can_present, uint32_t index)
        : m_device{ device }
        , m_family_index{ family_index }
        , m_index{ index }
        , m_can_present{ can_present }
        , m_properties{ properties }
    {
        vkGetDeviceQueue(device.get_handle(), family_index, index, &m_handle);
    }

    Queue::Queue(Queue&& other)
        : m_device{ other.m_device }
        , m_handle{ other.m_handle }
        , m_family_index{ other.m_family_index }
        , m_index{ other.m_index }
        , m_can_present{ other.m_can_present }
        , m_properties{ other.m_properties 
}
    {
        other.m_handle = VK_NULL_HANDLE;
        other.m_family_index = {};
        other.m_properties = {};
        other.m_can_present = VK_FALSE;
        other.m_index = 0;
    }

    Queue::~Queue() {
        // TODO destroy queue? Maybe not needed
    }
}