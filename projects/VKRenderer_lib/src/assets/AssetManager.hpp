#pragma once

#include <core/VertexData.h>

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

	// TODO fix ids for serialization
	//      ATM, ids are simple incrementail counters. We already have maps to store assets,
	//      what we need is a fast and safe way to get a new free id
	typedef uint32_t IdAssetMesh;
	typedef uint32_t IdAssetTexture;
	typedef uint32_t IdAssetMaterial;

	namespace BuiltinPrimitives {
		// TODO better indices for mesh and texture assets (separated from debug/builtin ones)
		const IdAssetMesh IDX_DEBUG_CUBE = 0;
		const IdAssetMesh IDX_DEBUG_RAY = 1;
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
	};

	struct MaterialData {
		// TODO replace with IdRenderPass and IdPipeline
		uint32_t id_render_pass;
		uint32_t id_pipeline;
		void* uniform_data_material;
		std::vector<VkImageView> image_views;
	};

	void asset_manager_init();

	uint32_t get_num_mesh_assets();
	uint32_t get_num_texture_assets();

	MeshData& get_mesh_data(IdAssetMesh id);
	TextureData& get_texture_data(IdAssetTexture id);
	MaterialData& get_material_data(IdAssetMaterial id);

	// ===================================================================================
	// load
	// ===================================================================================
	IdAssetMesh load_mesh(const char* path);
	std::vector<IdAssetMesh> load_meshes(const char** paths);
	std::vector<IdAssetMesh> load_meshes_from_folder(const char* folder_path);

	IdAssetTexture load_texture(const char* path, TexChannelTypes channels);

	// ===================================================================================
	// create
	// ===================================================================================

	// resources inside MeshData will be acquired by the Asset Manager system
	IdAssetMesh create_mesh(MeshData data);

	// resources inside MaterialData will be acquired by the Asset Manager system
	IdAssetMaterial create_material(MaterialData data);
}