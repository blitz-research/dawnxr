Dawn / openxr interop library

Adds a dawn openxr platform that should work the same as existing d3d12, vulkan etc openxr
platforms. 'Steals' the d3d11 platform defines to achieve this though.

Must be used with this fork of dawn: https://github.com/blitz-research/dawn

Uses the webgpu dawn c++ wrappers.

Only provides backends for D3D12 and Vulkan.

Only tested on Windows.

```
namespace dawnxr {

// Mirrors the XrGraphicsBindingD3D12KHR etc structs.
struct GraphicsBindingDawn {
	XrStructureType type = XR_TYPE_GRAPHICS_BINDING_DAWN_EXT;
	const void* XR_MAY_ALIAS next = nullptr;
	wgpu::Device device;
};

// Mirrors the XrSwapchainImageD3D12KHR etc structs.
struct SwapchainImageDawn {
	XrStructureType type = XR_TYPE_SWAPCHAIN_IMAGE_DAWN_EXT;
	void* XR_MAY_ALIAS next = nullptr;
	wgpu::TextureView textureView;
};

// Mirrors the XrGraphicsRequirementsD3D12KHR etc structs.
struct GraphicsRequirementsDawn {
	XrStructureType type = XR_TYPE_GRAPHICS_REQUIREMENTS_DAWN_EXT;
	void* XR_MAY_ALIAS next = nullptr;
};

// Gets dawn graphics requirements for a given backend type. Currently just dumps backend requirements to stdout.
XrResult getGraphicsRequirements(XrInstance instance, XrSystemId systemId, wgpu::BackendType backendType,
								 GraphicsRequirementsDawn* graphicsRequirements);

// Creates a dawn::native::AdapterDiscoveryOptionsBase subclass instance for a given backend type.
XrResult createAdapterDiscoveryOptions(XrInstance instance, XrSystemId systemId, wgpu::BackendType backendType,
									   dawn::native::AdapterDiscoveryOptionsBase** options);

// Use this instead of xrCreateSession
XrResult createSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session);

// Use this instead of xrDestroySession
XrResult destroySession(XrSession session);

// Use this instead of xrEnumerateSwapchainFormats
XrResult enumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput,
								   int64_t* formats);

// Use this instead of xrCreateSwapChain
XrResult createSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain);

// Use this instead of xrDestroySwapChain
XrResult destroySwapchain(XrSwapchain swapchain);

// Use this instead of xrEnumerateSwapchainImages
XrResult enumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput,
								  XrSwapchainImageBaseHeader* images);

} // namespace dawnxr
```

In practice, you can use it something like this:

```
wgpu::Device createXRCompatibleDevice(wgpu::BackendType backendType) {

	if (XR_FAILED(createXRInstance())) return {};

	dawn::native::AdapterDiscoveryOptionsBase* options;
	if (XR_FAILED(dawnxr::createAdapterDiscoveryOptions(g_instance, g_systemId, backendType, &options))) return {};

	// normal dawn device creation code...
	return createDevice(options);
}
```

Note that the createAdapterDiscoveryOptions call is only really necessary for Vulkan, OpenXR will generally 'just work'
with
D3D12 as is. 
