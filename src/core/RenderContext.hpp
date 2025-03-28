#pragma once

#include <vulkan/vulkan.h>

#include <core/Swapchain.hpp>
#include <core/RenderFrame.hpp>
#include <core/RenderPass.hpp>

#include <memory>

namespace TMP_Assets {
	typedef enum : uint8_t {
		TEX_CHANNELS_NONE = 0x0,
		TEX_CHANNELS_GREY = 0x1,
		TEX_CHANNELS_GREY_A = 0x2,
		TEX_CHANNELS_RGB = 0x3,
		TEX_CHANNELS_RGB_A = 0x4
	} TexChannelTypes;

	typedef struct {
		std::vector<VertexData> vertices;
		std::vector<uint32_t> indices;
	} MeshData;

	typedef struct {
		uint16_t width;
		uint16_t height;
		uint8_t channelsCount;
		TexChannelTypes channels;
		std::vector<unsigned char> data;
	} TextureData;

	TextureData get_texture_data();
	MeshData& get_mesh_data(uint32_t index);
}

namespace vkc {
	class Window;
	class PhysicalDevice;

	class RenderContext {
	public:
		RenderContext(
			const PhysicalDevice *physical_device,
			VkDevice device,
			VkSurfaceKHR surface,
			Window * window
		);
		~RenderContext();

		uint32_t get_num_render_frames() const { return m_frames.size(); };
		VkFormat get_swapchain_image_format() const { return m_swapchain->get_image_format(); };
		VkExtent2D get_swapchain_extent() const { return m_swapchain->get_extent(); };
		const Swapchain* get_obj_swapchain() const { return m_swapchain.get(); };
		const VkPhysicalDeviceProperties& get_physical_device_properties() const;

		void render_begin();
		void render_finalize();
		void recreate_swapchain();

		// memory utils
		void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
		void copy_buffer_to_image(
			VkBuffer buffer,
			VkImage image,
			uint32_t width,
			uint32_t height
		);
		void createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer* pBuffer,
			VkDeviceMemory* pBufferMemory
		);
		void transition_image_layout(
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout
		);
		void create_image(
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkImage* pImage,
			VkDeviceMemory* pImageMemory
		);
		VkImageView create_imge_view(
			VkImage image,
			VkFormat format,
			VkImageAspectFlags aspectFlags
		);

	private:
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		Window * m_window;
		const PhysicalDevice * m_obj_physical_device;
		VkDevice m_device;

		/// If swapchain exists, then this will be a present supported queue, else a graphics queue
		VkQueue m_queue_graphic;
		VkQueue m_queue_present;

		std::unique_ptr<vkc::Swapchain> m_swapchain;


		std::vector<std::unique_ptr<RenderFrame>> m_frames;
		std::vector<std::unique_ptr<RenderPass>> m_render_passes;

		// pools
		VkCommandPool m_command_pool;

		/// Current active frame index
		uint32_t m_active_frame_index{ 0 };
	};
}