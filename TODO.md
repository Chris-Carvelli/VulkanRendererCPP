## Notes
- FAVOR NON-INTERLEAVED VERTEX DATA LAYOUT, EASIER TO STORE DATA IN A PIPELINE-INDEPENDET STORAGE
- `IdAsset` and `IdGPUData` do no necessarily match (we have on the fly GPU data like procedural and debug meshes)

```c++
// format-independent MeshData
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
}

struct MeshData {
	void*			vertex_data;
	uint32_t		vertex_count;
	VertexAttrib*	vertex_attributes
	uint32_t		vertex_attributes_count
	uint32_t*		index_data
	uint32_t		index_count
	// [optional]	enum for contiguous ot interlieaved vertex data storage
}
```

## TODO
- [ ] - long term
	- [x] figure out if we have one framebuffer per swapchain, one per renderpass, or both
	- [ ] rework file reading (no string, no std)
	- [ ] set default shader texture as separate image and sampler (see https://docs.vulkan.org/samples/latest/samples/api/separate_image_sampler/README.html)
		- [ ] actually, now we know that we can update descriptors, so probably not needed
	- [ ] record draw calls, obtain descriptor sets from rendercontext
	- [x] create a pipeline with attachemnts, state and stuff
	- [x] submit a pipeline, replicate all needed attachments over all RenderFrames
	- [ ] resource management (keep texture/mesh data, for mapping later)
	- [x] proper allocation of all vulkan objects
		- [x] RenderFrame (TMPs)
		- [x] RenderFrame (class)
		- [x] RenderContext
		- [x] Swapchain
		- [x] Instance
		- [x] VKRenderer
	- [ ] centralize and uniform num frames in flight (`RenderContext` has a const in its constructor, Swapchain gets it from the capabilities, unsure what others do)
	- [x] proper cleanup of all vulkan objects
	- [ ] uniform naming conventions
		- [ ] rename camel case symbols
		- [ ] prefix `handle` to all vulkan objects
		- [ ] prefix `obj` to all vkc object pointers
	- [ ] use `CC_VK_CHECK` macro on all object (eventually, add an optional log message in case of error if needed)
	- [ ] fix the super mangled Instance/PhysicalDevice/Surface/Device mess for the graphics queue
	- [x] do attachments belong to pipeline or renderpass? (probably renderpass. But it also depends between inputs and outputs?)
		- [x] A: output to renderpass, inputs to pipeline
	- [ ] implement `findDepthFormat()`
	- [ ] check if we want/need anisotropy. If so, request and enable it in physical/logical device
	- [ ] proper handling of indexed meshes
	- [x] VK objects cleanup
	- [x] setup to have one buffer per object, always on GPU
		- [x] map all resources on app creation
		- [x] rendercalls reference bufers4
	- [ ] setup to have one UBO, one VBO and one EBO
		- [ ] command buffers for moving data from CPU to GPU
		- [ ] `drawIndirect`
	- [ ] compare approaches
	- [ ] compile assets (png, glTF, glsl are in `res`, compiled version in `out`, get rid of the annoying `currDir` in VS launch config)
	- [ ] fix tris winding order
	- [ ] refactor imgui integration
		- [ ] needs to be called in `RenderFrame`, ugly singleton
		- [x] check why it creates/destroys cmb buffer at inappropriate times
				(SOLUTION: one of ImGui's Init parameters is `imageCount`, which stands for SWAPCHAIN image count. That was not set correctly)
	- [x] refactor pipelines
		- [ ] ~handle multiple uniform buffer and push constants~
		- [x] number and size of buffer is a creation parameter
	- [ ] check why adding light color to blinn phong hue shifts (???) the final color
	- [ ] add debug names to assets and buffers

- 25/04/25
	- [x] move character
	- [ ] draw trail
		- [x] create pipeline with TRI_STRIP topology
		- [x] write shader for billboarded triangle strip
		- [x] upload X vertives to the GPU
		- [x] set vertex attributes init time
		- [x] update vertex position at runtime
		- [ ] switch offsets based on frame to avoid flickering
	- [ ] main application can create renderpasses
	- [ ] main application can create pipelines
		- [ ] `PipelineConfig` should contain `IdAssetTexture` rather than texture view

- 15/04/25
	- [x] remove `TMP_Math` namespace
	- [x] fix `TMP_Assets` namespace
	- [x] fix `TMP_Update` namespace
		- [x] fix ImGui singleton (still a singleton, but less intrusive)
		- [x] add light controls
		- [x] add camera controls
		- [x] record GUI changes in appropriate place

- 14/04/25
	- [ ] mouse hovering models
		- [x] implement `add_debug_ray()`
		- [ ] change color of box when ray intersects it
		- [ ] cast ray from mouse posiyion
	- [x] fix camera controls
	- [ ] create bounding boxes for each mesh
	- [ ] add outline to hovered mesh
	- [ ] fix `TMP_Assets` namespace
	- [ ] fix `TMP_Update` namespace
	- [ ] design and implement surface API

- 13/04/25
	- [x] figure out why `DebugRenderPipeline` tries to unterpret vertex buffer as full `VertexData`, instead of `glm::vec3`
		  (A: it's because the binding descriptors specify the stride, and we were using `sizeof(VertexData)`)

- 09/04/25
	- [x] improve "editor" camera
	- [x] basic bounding boxes
	- [ ] raycast from mouse to boxes
	- [ ] space partitioning

- 04/04/25
	- [x] shader compile script
	- [x] fixed imgui errors
	- [ ] fix pipeline mess
		- [x] pipeline config
		- [x] pipeline vertex input + descriptors
		- [ ] set update descriptor bindings
	- [x] figure out next steps
	- [x] store image textures
		- [x] map of <id, { image, memory, view }>
	- [ ] pass image views to `PipelineConfig`
	- [ ] add option to specify sampling info in a par-texture basis in `Pipeline`
	- [ ] ~add option to manually update `Pipeline`'s bindings (needed for render textures)~
			(would need multiple renderpasses, with the ability to customize the framebuffer creation AND handle render textures resizing with swapchain. out of Scope)

- 03/04/25
	- [x] re-enable imgui
	- [ ] add forward lighting
		- [x] add light data to frame UBO
		- [x] implement blinn-phong
		- [x] update light data realtime
		- [x] add light controls to GUI
		- [ ] ~#include UBOs in vertex and fragment shaders~
	- [ ] add SS outline
	- [ ] outline hoveres mesh
	- [ ] shadow pass

- 29/03/25
	- [x] rename one of the two `ModelData` (used for both mesh info AND push constants, no good)
	- [x] test multiple renderpasses (pipelines)
		- [x] create arrays of `BeginRenderPassInfo` and `VkPipeline`
		- [x] add ids/indices of render pass and pipeline into drawcall
		- [ ] sort drawcalls
		- [x] create outline shader
			- [x] add normals to model data (instead of color)
			- [x] write shader
		- [x] create new pipeline
			- [x] shaders as inputs?
	- [x] plan next mid-term goal

- 28/03/25
	- [x] `RenderPass` initialized a single `Pipeline`
	- [x] `RenderContext` own at least one `RenderPass`
	- [x] `RenderContext` passes rende rpass and pipeline info to `RenderFrame`
	- [x] `RenderContext` handles rendering of Application objects (first step towards DrawCalls)
		- [x] `RenderContext` loads asset data (mesh and texture)
		- [x] `RenderContext` create VK objects from asset data
		- [x] fix remaining problems and test naive loop
	- [ ] test multiple objects
		- [x] map of assets
		- [x] load tiles
		- [x] loop over all objects
		- [x] debug
			- [x] mesh path is wrong (name only, missing dir)
			- [x] naive setup calling `RnderFrame::render` for every drawcall does not work. Need to pass all the drawcalls at the same time
		- [x] test push constants
			- [x] modify shaders
			- [x] modify `VertexData.h`
			- [x] set constants
			- [x] record multiple cmd buffers in `RnderFrame::render`
			- [x] debug
		- [ ] polish
			- [x] create objects centered
			- [x] fix object orientation on loading
			- [x] test with MORE drawcalls
			- [x] load entire asset pack

- 21/03/25
	- [x] check order of VK object initialization
		- [x] fix shader names
		- [x] fix read_file_binary
		- [x] initialize `descriptorSetLayout` correctly
		- [x] init queues before `RenderFrame`
		- [x] fix texture paths
	- [x] chech order of VK calls during render
		- [x] had the assumption of using a single cmd buffer per frame, but the VK tutorial was set up to use a cmd buffer per CALL.
				make it work for a single drawcall for now
	- [x] fix texture mapping Y coord
	- [x] fix `failed to allocate buffer memory!` error (model loader creates way too much data, comapred C renderer)
	- [x] fix aspect ratio (updateUniform buffer needs screen size to compute proj matrix correctly)
	- [x] check uniform update with model rotate
	- [x] check window resizing
		- [x] ~possible solution: `vkGetPhysicalDeviceImageFormatProperties` may be updated when the window changes, try to call it every time on swapchain recreation~
		- [x] update swapchain capabilities evey time the swapchain is recreated
	- [ ] render multiple objects
		- [ ] duplicate object resources (ubo, vbo, ebo, textures)
		- [ ] make two calls to `recordCmdBuffer`
	- [ ] render two objects in the same drawcall/command buffer
	

- 20/03/05
	- [x] update/bind ubo
	- [x] update/bind vbo
	- [x] update/bind ebo
	- [x] allocate mesh data
	- [x] load mesh data
	- [x] allocate/initialize all resources
	- [x] allocate/bind albedo texture
	- [x] initialize Vulkan objects
		- [x] RenderFrame (TMPs)
		- [x] RenderFrame (class)
		- [x] RenderContext
			- [x] window
			- [x] physical device
			- [x] device
			- [x] queues
			- [x] swapchain
			- [x] render frames
			- [x] command pool

- 19/03/25
	

- 14/03/25
	- [x] handle descriptors
		- [x] pool (RenderContext)
		- [x] sets (application)
		- [x] set layouts (application))

- 11/03/25
	- [x] logical device
	
## Drawcalls
```c
struct {
	VkRenderPass pass;
	Vkpipeline pipeline;
	Drawcall call;
}

sorting:
pass	pipeline	call
0		opaque		0
0		opaque		1
0		opaque		2
0		transparent	0
0		transparent	1
1		hair		2
```
Pipelines belong to a pass, but calls can be drawn multiple times with different pipelines/passes.
Calls need to be sorted appropriately in order to minimize pipeline/pass swiches.