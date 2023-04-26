#include "dawnxr_internal.h"

#include <unordered_map>

#include <iostream>

using namespace dawnxr::internal;

namespace {

struct Swapchain {
	XrSwapchain const backendSwapchain;
	Session* const session;
	std::vector<wgpu::TextureView> const images;
};

std::unordered_map<XrSession, Session*> g_sessions;

std::unordered_map<XrSwapchain, Swapchain*> g_swapchains;

} // namespace

namespace dawnxr {

XrResult getGraphicsRequirements(XrInstance instance, XrSystemId systemId, wgpu::BackendType backendType,
								 GraphicsRequirementsDawn* requirements) {

	if (requirements->type != XR_TYPE_GRAPHICS_REQUIREMENTS_DAWN_EXT) return XR_ERROR_HANDLE_INVALID;

	switch (backendType) {
#ifdef XR_USE_GRAPHICS_API_D3D12
	case wgpu::BackendType::D3D12:
		XR_TRY(getD3D12GraphicsRequirements(instance, systemId, requirements));
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
	case wgpu::BackendType::Vulkan:
		XR_TRY(getVulkanGraphicsRequirements(instance, systemId, requirements));
		break;
#endif
	default:
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return XR_SUCCESS;
}

XrResult createAdapterDiscoveryOptions(XrInstance instance, XrSystemId systemId, wgpu::BackendType backendType,
									   dawn::native::AdapterDiscoveryOptionsBase** options) {

	switch (backendType) {
#ifdef XR_USE_GRAPHICS_API_D3D12
	case wgpu::BackendType::D3D12:
		XR_TRY(createD3D12AdapterDiscoveryOptions(instance, systemId, options));
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
	case wgpu::BackendType::Vulkan:
		XR_TRY(createVulkanAdapterDiscoveryOptions(instance, systemId, options));
		break;
#endif
	default:
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return XR_SUCCESS;
}

XrResult createSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session) {

	auto binding = (GraphicsBindingDawn*)createInfo->next;
	if (binding->type != XR_TYPE_GRAPHICS_BINDING_DAWN_EXT) return xrCreateSession(instance, createInfo, session);

	auto backendType =
		(wgpu::BackendType)dawn::native::GetWGPUBackendType(dawn::native::GetWGPUAdapter(binding->device.Get()));

	// TODO: Woah, you *HAVE* to get graphics requirements or session creation fails?!?
	//
	GraphicsRequirementsDawn requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_DAWN_EXT};
	XR_TRY(getGraphicsRequirements(instance, createInfo->systemId, backendType, &requirements));

	Session* dawnSession;

	switch (backendType) {
#ifdef XR_USE_GRAPHICS_API_D3D12
	case wgpu::BackendType::D3D12:
		XR_TRY(createD3D12Session(instance, createInfo, &dawnSession));
		break;
#endif
#ifdef XR_USE_GRAPHICS_API_VULKAN
	case wgpu::BackendType::Vulkan:
		XR_TRY(createVulkanSession(instance, createInfo, &dawnSession));
		break;
#endif
	default:
		return XR_ERROR_RUNTIME_FAILURE;
	}

	*session = dawnSession->backendSession;
	g_sessions.insert(std::make_pair(*session, dawnSession));

	return XR_SUCCESS;
}

XrResult destroySession(XrSession session) {

	// TODO: What happens if a session is delete before its swapchains
	auto it = g_sessions.find(session);
	if (it != g_sessions.end()) {
		delete it->second;
		g_sessions.erase(it);
	}
	return xrDestroySession(session);
}

XrResult enumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput,
								   int64_t* formats) {

	auto it = g_sessions.find(session);
	if (it == g_sessions.end()) { //
		return xrEnumerateSwapchainFormats(session, formatCapacityInput, formatCountOutput, formats);
	}
	auto dawnSession = it->second;

	std::vector<wgpu::TextureFormat> dawnFormats;
	XR_TRY(dawnSession->enumerateSwapchainFormats(dawnFormats));

	*formatCountOutput = (uint32_t)dawnFormats.size();

	if (formats) {
		auto n = std::min(formatCapacityInput, *formatCountOutput);
		for (auto i = 0u; i < n; ++i) formats[i] = (int64_t)dawnFormats[i];
	}

	return XR_SUCCESS;
}

XrResult createSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain) {

	auto it = g_sessions.find(session);
	if (it == g_sessions.end()) return xrCreateSwapchain(session, createInfo, swapchain);
	auto dawnSession = it->second;

	std::vector<wgpu::TextureView> images;
	XR_TRY(dawnSession->createSwapchain(createInfo, images, swapchain));

	auto dawnSwapchain = new Swapchain{*swapchain, dawnSession, std::move(images)};
	g_swapchains.insert(std::make_pair(*swapchain, dawnSwapchain));

	return XR_SUCCESS;
}

XrResult destroySwapchain(XrSwapchain swapchain) {

	auto it = g_swapchains.find(swapchain);
	if (it != g_swapchains.end()) {
		auto dawnSwapchain = it->second;
		// TODO: Need to destroy swapchain image wrappers
		// dawnSwapchain->session->destroySwapchainImages();
		g_swapchains.erase(it);
		delete dawnSwapchain;
	}

	return xrDestroySwapchain(swapchain);
}

XrResult enumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput,
								  XrSwapchainImageBaseHeader* images) {

	auto it = g_swapchains.find(swapchain);
	if (it == g_swapchains.end()) { //
		return xrEnumerateSwapchainImages(swapchain, imageCapacityInput, imageCountOutput, images);
	}

	auto dawnSwapchain = it->second;

	*imageCountOutput = (uint32_t)dawnSwapchain->images.size();

	if (images) {
		auto n = std::min(imageCapacityInput, *imageCountOutput);
		auto dawnImages = (SwapchainImageDawn*)images;
		for (auto i = 0u; i < n; ++i) {
			if (dawnImages[i].type != XR_TYPE_SWAPCHAIN_IMAGE_DAWN_EXT) return XR_ERROR_HANDLE_INVALID;
			dawnImages[i].textureView = dawnSwapchain->images[i];
		}
	}

	return XR_SUCCESS;
}

} // namespace dawnxr
