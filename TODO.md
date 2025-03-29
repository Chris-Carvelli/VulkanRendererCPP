- long term
	- [x] figure out if we have one framebuffer per swapchain, one per renderpass, or both
	- [ ] rework file reading (no string, no std)
	- [ ] set default shader texture as separate image and sampler (see https://docs.vulkan.org/samples/latest/samples/api/separate_image_sampler/README.html)
	- [ ] record draw calls, obtain descriptor sets from rendercontext
	- [ ] create a pipeline with attachemnts, state and stuff
	- [ ] submit a pipeline, replicate all needed attachments over all RenderFrames
	- [ ] resource management (keep texture/mesh data, for mapping later)
	- [x] proper allocation of all vulkan objects
		- [x] RenderFrame (TMPs)
		- [x] RenderFrame (class)
		- [x] RenderContext
		- [x] Swapchain
		- [x] Instance
		- [x] VKRenderer
	- [ ] centralize and uniform num frames in flight (`RenderContext` has a const in its constructor, Swapchain gets it from the capabilities, unsure what others do)
	- [ ] proper cleanup of all vulkan objects
	- [ ] uniform naming conventions
		- [ ] rename camel case symbols
		- [ ] prefix `handle` to all vulkan objects
		- [ ] prefix `obj` to all vkc object pointers
	- [ ] use `CC_VK_CHECK` macro on all object (eventually, add an optional log message in case of error if needed)
	- [ ] fix the super mangled Instance/PhysicalDevice/Surface/Device mess for the graphics queue
	- [ ] do attachments belong to pipeline or renderpass? (probably renderpass. But it also depends between inputs and outputs?)
	- [ ] implement `findDepthFormat()`
	- [ ] check if we want/need anisotropy. If so, request and enable it in physical/logical device
	- [ ] proper handling of indexed meshes
	- [x] VK objects cleanup
	- [ ] setup to have one buffer per object, always on GPU
		- [ ] map all resources on app creation
		- [ ] rendercalls reference bufers4
	- [ ] setup to have ono UBO, une VBO and one EBO
		- [ ] command buffers for moving data from CPU to GPU
		- [ ] `drawIndirect`
	- [ ] compare approaches

- 29/03/25
	- [x] rename one of the two `ModelData` (used for both mesh info AND push constants, no good)
	- [ ] test multiple renderpasses
	- [ ] plan next mid-term goal

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