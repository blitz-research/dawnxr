#pragma once

#include "dawnxr.h"

#include <dawn/webgpu.h>

#include <vector>

#define XR_TRY(X)                                                                                                      \
	{                                                                                                                  \
		auto r = (X);                                                                                                  \
		if (XR_FAILED(r)) {                                                                                            \
			std::cout << "### XR_TRY failed: " << #X << std::endl;                                                     \
			return r;                                                                                                  \
		}                                                                                                              \
	}

#define XR_PROC(XRINST, FUNCID)                                                                                        \
	PFN_##FUNCID FUNCID{};                                                                                             \
	xrGetInstanceProcAddr((XRINST), #FUNCID, (PFN_xrVoidFunction*)(&FUNCID));

namespace dawnxr::internal {

struct Session {

	XrSession const backendSession;
	WGPUDevice const device;

	virtual XrResult enumerateSwapchainFormats(std::vector<WGPUTextureFormat>& formats) = 0;

	virtual XrResult createSwapchain(const XrSwapchainCreateInfo* createInfo, std::vector<WGPUTextureView>& images,
									 XrSwapchain* swapchain) = 0;

	// TODO: destroySwapchainImages

	virtual ~Session() = default;

protected:
	Session(XrSession session, WGPUDevice device) : backendSession(session), device(device) {}
};

#ifdef XR_USE_GRAPHICS_API_D3D12
XrResult getD3D12GraphicsRequirements(XrInstance instance, XrSystemId systemId, GraphicsRequirementsDawn* requirements);
XrResult createD3D12AdapterDiscoveryOptions(XrInstance instance, XrSystemId systemId,
											dawn::native::AdapterDiscoveryOptionsBase** options);
XrResult createD3D12Session(XrInstance instance, const XrSessionCreateInfo* createInfo, Session** session);
#endif

#ifdef XR_USE_GRAPHICS_API_VULKAN
XrResult getVulkanGraphicsRequirements(XrInstance instance, XrSystemId systemId,
									   GraphicsRequirementsDawn* requirements);
XrResult createVulkanAdapterDiscoveryOptions(XrInstance instance, XrSystemId systemId,
											 dawn::native::AdapterDiscoveryOptionsBase** options);
XrResult createVulkanSession(XrInstance instance, const XrSessionCreateInfo* createInfo, Session** session);
#endif

} // namespace dawnxr::internal
