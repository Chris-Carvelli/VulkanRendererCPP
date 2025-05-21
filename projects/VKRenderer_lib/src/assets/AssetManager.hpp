#pragma once

#include <core/VertexData.h>
#include <cc_logger.h>
#include <core/Pipeline.hpp>
#include <vector>

namespace vkc::Assets {
	enum VertexAttrib : uint8_t {
		POSITION,
		NORMAL,
		TANGENT,
		COLOR_0,
		COLOR_1,
		COLOR_2,
		COLOR_3,
		COLOR_4,
		COLOR_5,
		COLOR_6,
		COLOR_7,
		TEX_COORDS_0,
		TEX_COORDS_1,
		TEX_COORDS_2,
		TEX_COORDS_3,
		TEX_COORDS_4,
		TEX_COORDS_5,
		TEX_COORDS_6,
		TEX_COORDS_7
	};

	typedef enum : uint8_t {
		TEX_CHANNELS_NONE = 0x0,
		TEX_CHANNELS_GREY = 0x1,
		TEX_CHANNELS_GREY_A = 0x2,
		TEX_CHANNELS_RGB = 0x3,
		TEX_CHANNELS_RGB_A = 0x4
	} TexChannelTypes;

	typedef enum : uint8_t {
		TEX_VIEW_TYPE_2D   = VK_IMAGE_VIEW_TYPE_2D,
		TEX_VIEW_TYPE_CUBE = VK_IMAGE_VIEW_TYPE_CUBE
	} TexViewTypes;

	// TODO fix ids for serialization
	//      ATM, ids are simple incrementail counters. We already have maps to store assets,
	//      what we need is a fast and safe way to get a new free id
	typedef uint32_t IdAssetMesh;
	typedef uint32_t IdAssetTexture;
	typedef uint32_t IdAssetMaterial;

	const IdAssetTexture IDX_MISSING_TEXTURE = -1;

	inline uint8_t tex_get_num_channels(TexChannelTypes t) {
		switch(t) {
			case TEX_CHANNELS_GREY   : return 1;
			case TEX_CHANNELS_GREY_A : return 2;
			case TEX_CHANNELS_RGB    : return 3;
			case TEX_CHANNELS_RGB_A  : return 4;
			case TEX_CHANNELS_NONE   : CC_ASSERT(false, "Invalid texture channel type");
		}
	}
	namespace BuiltinPrimitives {
		// TODO better indices for mesh and texture assets (separated from debug/builtin ones)
		const IdAssetMesh IDX_DEBUG_CUBE = 0;
		const IdAssetMesh IDX_DEBUG_RAY  = 1;

		const IdAssetMesh IDX_FULLSCREEN_TRI = 2;

		const IdAssetTexture IDX_TEX_WHITE     = 0;
		const IdAssetTexture IDX_TEX_BLACK     = 1;
		const IdAssetTexture IDX_TEX_BLUE_NORM = 2;
	}

	struct MeshData {
		static const uint8_t	FLAG_DYNAMIC = 0x00000001;

		void*			vertex_data;
		uint32_t		vertex_count;
		uint32_t		vertex_data_size; // size of a single element
		uint32_t*		index_data;
		uint32_t		index_count;
		// [optional]	enum for contiguous ot interlieaved vertex data storage
		uint8_t			flags;
	};

	struct TextureData {
		uint16_t width;
		uint16_t height;
		uint8_t channelsCount;
		TexChannelTypes channels;
		std::vector<unsigned char> data;
		VkImageViewType viewType;
		VkFormat format;
	};

	struct MaterialData {
		uint32_t id_pipeline_config;
		// TODO replace with IdRenderPass and IdPipeline
		uint32_t id_render_pass;
		uint32_t id_pipeline;
		void* uniform_data_material;
		std::vector<IdAssetTexture> image_views;
	};

	void asset_manager_init();

	uint32_t get_num_mesh_assets();
	uint32_t get_num_texture_assets();
	uint32_t get_num_material_assets();

	MeshData& get_mesh_data(IdAssetMesh id);
	TextureData& get_texture_data(IdAssetTexture id);
	MaterialData& get_material_data(IdAssetMaterial id);

	// ===================================================================================
	// load
	// ===================================================================================
	IdAssetMesh load_mesh(const char* path);
	IdAssetMesh load_mesh(const char* path, const char* path_material);
	std::vector<IdAssetMesh> load_meshes(const char** paths);
	std::vector<IdAssetMesh> load_meshes_from_folder(const char* folder_path);

	uint32_t load_model(
		const char* path,
		const char* base_path_textures,
		IdAssetTexture TMP_tex_environment_id,
		std::vector<IdAssetMesh>&     out_loaded_mesh_idxs,
		std::vector<IdAssetMaterial>& out_loaded_materials_idxs
	);

	IdAssetTexture load_texture(const char* path, TexChannelTypes channels, TexViewTypes viewType = TEX_VIEW_TYPE_2D, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, bool flip_vertical=false);

	// ===================================================================================
	// create
	// ===================================================================================

	// resources inside MeshData will be acquired by the Asset Manager system
	IdAssetMesh create_mesh(MeshData data);

	// resources inside MaterialData will be acquired by the Asset Manager system
	IdAssetMaterial create_material(MaterialData& data);
}