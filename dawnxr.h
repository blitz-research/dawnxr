#pragma once

#include <dawn/webgpu.h>

#include <dawn/native/DawnNative.h>

// Currently supported backends: D3D12, Vulkan

#if _WIN32
#define XR_USE_GRAPHICS_API_D3D12 1
#include <d3d12.h>
#include <windows.h>
#undef max
#undef min
#define XR_USE_GRAPHICS_API_VULKAN 1
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan.h>
#else
#error System not supported
#endif

#include <openxr/openxr_platform.h>

// Borrow the D3D11 enums for now
#define XR_TYPE_GRAPHICS_BINDING_DAWN_EXT XR_TYPE_GRAPHICS_BINDING_D3D11_KHR
#define XR_TYPE_SWAPCHAIN_IMAGE_DAWN_EXT XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR
#define XR_TYPE_GRAPHICS_REQUIREMENTS_DAWN_EXT XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR

namespace dawnxr {

// Mirrors the XrGraphicsBindingD3D12KHR etc structs.
struct GraphicsBindingDawn {
	XrStructureType type;
	const void* XR_MAY_ALIAS next;
	WGPUDevice device;
};

// Mirrors the XrSwapchainImageD3D12 etc structs.
struct SwapchainImageDawn {
	XrStructureType type;
	void* XR_MAY_ALIAS next;
	WGPUTextureView textureView;
};

// Mirrors the XrGraphicsRequirementsD3D12KHR etc structs.
struct GraphicsRequirementsDawn {
	XrStructureType type;
	void* XR_MAY_ALIAS next;
};

// Gets dawn graphics requirements for a given backend type. Currently just dumps backend requirements to stdout.
XrResult getGraphicsRequirements(XrInstance instance, XrSystemId systemId, WGPUBackendType backendType,
								 GraphicsRequirementsDawn* graphicsRequirements);

// Creates a dawn::native::AdapterDiscoveryOptionsBase subclass instance for a given backend type.
XrResult createAdapterDiscoveryOptions(XrInstance instance, XrSystemId systemId, WGPUBackendType backendType,
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
