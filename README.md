Dawn / OpenXR interop library.

Adds a dawn platform to OpenXR that should work the same as the existing D3D12, Vulkan etc platforms.

*Must* be used with the 'openxr-dev' branch of this fork of dawn: https://github.com/blitz-research/dawn

Steals the D3D11 platform type defines.
 
Uses the webgpu dawn c++ wrappers.

Only supports D3D12 and Vulkan.

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
XrResult createOpenXRConfig(XrInstance instance, XrSystemId systemId, wgpu::BackendType backendType, void** config);

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

To create an XR compatible Vulkan dawn device (a D3D12 dawn device should just work 'as is' with the above functions):

```
wgpu::Device createXRCompatibleDevice(wgpu::BackendType backendType) {

    WB_ASSERT(backendType==wgpu::BackendType::D3D12 || backendType==wgpu::BackendType::Vulkan);

	if (XR_FAILED(createXrInstance())) return {};

	wgpu::RequestAdapterOptions adapterOpts{};
	dawn::native::vulkan::RequestAdapterOptionsOpenXRConfig adapterOptsXRConfig{};
	
	adapterOpts.backendType = backendType;
	
    // Don't need this for D3D12.
	if(backendType == wgpu::BackendType::Vulkan) {
		dawnxr::createOpenXRConfig(getXrInstance(), getXrSystemId(), backendType, (void**)&adapterOptsXRConfig.openXRConfig);
		adapterOpts.nextInChain = &adapterOptsXRConfig;
	}
	
	auto adapters = wgpuInstance->EnumerateAdapters(&adapterOpts);
	WB_ASSERT(!adapters.empty());
	
	auto device = adapters.front().CreateDevice();
	WB_ASSERT(device);

	return device;
}
```
