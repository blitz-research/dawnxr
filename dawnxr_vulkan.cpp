#include "dawnxr_internal.h"

#include <dawn/native/VulkanBackend.h>

#include <functional>
#include <iostream>
#include <vector>

using namespace dawnxr::internal;

namespace {

const auto dawnSwapchainFormat = WGPUTextureFormat_BGRA8UnormSrgb;
const auto vulkanSwapchainFormat = VK_FORMAT_B8G8R8A8_SRGB;

struct VulkanSession : Session {

	VulkanSession(XrSession session, WGPUDevice device) : Session(session, device) {}

	XrResult enumerateSwapchainFormats(std::vector<WGPUTextureFormat>& formats) override {

		formats.push_back(dawnSwapchainFormat);

		return XR_SUCCESS;
	}

	XrResult createSwapchain(const XrSwapchainCreateInfo* createInfo, std::vector<WGPUTextureView>& images,
							 XrSwapchain* swapchain) override {

		if (createInfo->type != XR_TYPE_SWAPCHAIN_CREATE_INFO) return XR_ERROR_HANDLE_INVALID;

		if (createInfo->format != dawnSwapchainFormat) return XR_ERROR_RUNTIME_FAILURE;

		auto vulkanInfo = *createInfo;
		vulkanInfo.format = vulkanSwapchainFormat;

		XR_TRY(xrCreateSwapchain(backendSession, &vulkanInfo, swapchain));

		// TODO: Need to cleanup swapchain if any of the below fails

		uint32_t n;

		XR_TRY(xrEnumerateSwapchainImages(*swapchain, 0, &n, nullptr));
		// XrSwapchainImageVulkan2KHR is an alias for XrSwapchainImageVulkanKHR
		std::vector<XrSwapchainImageVulkan2KHR> vulkanImages(
			n, XrSwapchainImageVulkan2KHR{XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
		XR_TRY(xrEnumerateSwapchainImages(*swapchain, n, &n, (XrSwapchainImageBaseHeader*)vulkanImages.data()));
		if (n != vulkanImages.size()) return XR_ERROR_RUNTIME_FAILURE;

		WGPUTextureDescriptor textureDesc{
			nullptr,												// nextInChain
			nullptr,												// label
			WGPUTextureUsage_RenderAttachment,						// usage
			WGPUTextureDimension_2D,								// dimension
			WGPUExtent3D{createInfo->width, createInfo->height, 1}, // size
			(WGPUTextureFormat)createInfo->format,					// format
			createInfo->mipCount,									// mipLevelCount;
			createInfo->sampleCount,								// sampleCount;
			0,														// viewFormatCount;
			nullptr													// view formats
		};

		WGPUTextureViewDescriptor textureViewDesc{
			nullptr,					 // nextInChain
			nullptr,					 // label
			textureDesc.format,			 // format
			WGPUTextureViewDimension_2D, // dimension
			0,							 // baseMipLevel
			1,							 // mipLevelCount
			0,							 // baseArrayLayer
			1,							 // arrayLayerCount
			WGPUTextureAspect_All		 // aspect
		};

		for (auto& it : vulkanImages) {
			auto texture = dawn::native::vulkan::CreateSwapchainWGPUTexture(device, &textureDesc, it.image);
			images.push_back(wgpuTextureCreateView(texture, &textureViewDesc));
		}

		return XR_SUCCESS;
	}
};

} // namespace

namespace dawnxr::internal {

XrResult getVulkanGraphicsRequirements(XrInstance instance, XrSystemId systemId,
									   GraphicsRequirementsDawn* requirements) {

	XR_PROC(instance, xrGetVulkanGraphicsRequirements2KHR);

	XrGraphicsRequirementsVulkan2KHR vulkanReqs{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
	XR_TRY(xrGetVulkanGraphicsRequirements2KHR(instance, systemId, &vulkanReqs));

	std::cout << "### Vulkan graphics requirements minApiVersionSupported: " << vulkanReqs.minApiVersionSupported
			  << std::endl;
	std::cout << "### Vulkan graphics requirements maxApiVersionSupported: " << vulkanReqs.maxApiVersionSupported
			  << std::endl;

	return XR_SUCCESS;
}

XrResult createVulkanAdapterDiscoveryOptions(XrInstance instance, XrSystemId systemId,
											 dawn::native::AdapterDiscoveryOptionsBase** options) {

	dawn::native::vulkan::OpenXRConfig xrConfig;

	xrConfig.enabled = true;

	xrConfig.CreateVkInstance = [=](PFN_vkGetInstanceProcAddr getProcAddr, const VkInstanceCreateInfo* vkCreateInfo,
									const VkAllocationCallbacks* vkAllocator, VkInstance* vkInstance) -> VkResult {
		XR_PROC(instance, xrCreateVulkanInstanceKHR);

		XrVulkanInstanceCreateInfoKHR createInfo{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
		createInfo.pfnGetInstanceProcAddr = getProcAddr;
		createInfo.vulkanCreateInfo = vkCreateInfo;
		createInfo.vulkanAllocator = vkAllocator;

		VkResult vkResult;
		auto r = xrCreateVulkanInstanceKHR(instance, &createInfo, vkInstance, &vkResult);
		if (XR_FAILED(r)) return VK_ERROR_UNKNOWN;
		return vkResult;
		return VK_SUCCESS;
	};

	xrConfig.GetVkPhysicalDevice = [=](VkInstance vkInstance, VkPhysicalDevice* vkPDevice) -> VkResult {
		XR_PROC(instance, xrGetVulkanGraphicsDevice2KHR);

		XrVulkanGraphicsDeviceGetInfoKHR getInfo{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
		getInfo.systemId = systemId;
		getInfo.vulkanInstance = vkInstance;

		auto r = xrGetVulkanGraphicsDevice2KHR(instance, &getInfo, vkPDevice);
		if (XR_FAILED(r)) return VK_ERROR_UNKNOWN;
		return VK_SUCCESS;
	};

	xrConfig.CreateVkDevice = [=](PFN_vkGetInstanceProcAddr getProcAddr, VkPhysicalDevice vkPDevice,
								  const VkDeviceCreateInfo* vkCreateInfo, const VkAllocationCallbacks* vkAllocator,
								  VkDevice* vkDevice) -> VkResult {
		XR_PROC(instance, xrCreateVulkanDeviceKHR);

		XrVulkanDeviceCreateInfoKHR createInfo{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
		createInfo.pfnGetInstanceProcAddr = getProcAddr;
		createInfo.vulkanPhysicalDevice = vkPDevice;
		createInfo.vulkanCreateInfo = vkCreateInfo;
		createInfo.vulkanAllocator = vkAllocator;

		VkResult vkResult;
		auto r = xrCreateVulkanDeviceKHR(instance, &createInfo, vkDevice, &vkResult);
		if (XR_FAILED(r)) return VK_ERROR_UNKNOWN;
		return vkResult;
	};

	auto discoveryOptions = new dawn::native::vulkan::AdapterDiscoveryOptions();
	discoveryOptions->openXRConfig = xrConfig;
	*options = discoveryOptions;
	return XR_SUCCESS;
}

XrResult createVulkanSession(XrInstance instance, const XrSessionCreateInfo* createInfo, Session** session) {

	if (createInfo->type != XR_TYPE_SESSION_CREATE_INFO) return XR_ERROR_HANDLE_INVALID;

	auto dawnBinding = (GraphicsBindingDawn*)createInfo->next;
	if (dawnBinding->type != XR_TYPE_GRAPHICS_BINDING_DAWN_EXT) return XR_ERROR_HANDLE_INVALID;
	auto dawnDevice = dawnBinding->device;

	// XrGraphicsBindingVulkan2KHR is an alias for XrGraphicsBindingVulkanKHR
	XrGraphicsBindingVulkan2KHR vulkanBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};

	vulkanBinding.instance = dawn::native::vulkan::GetInstance(dawnDevice);
	vulkanBinding.physicalDevice = dawn::native::vulkan::GetVkPhysicalDevice(dawnDevice);
	vulkanBinding.device = dawn::native::vulkan::GetVkDevice(dawnDevice);
	vulkanBinding.queueFamilyIndex = dawn::native::vulkan::GetGraphicsQueueFamily(dawnDevice);
	vulkanBinding.queueIndex = 0;

	XrSessionCreateInfo vulkanCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
	vulkanCreateInfo.next = &vulkanBinding;
	vulkanCreateInfo.systemId = createInfo->systemId;

	XrSession backendSession;
	XR_TRY(xrCreateSession(instance, &vulkanCreateInfo, &backendSession));
	*session = new VulkanSession(backendSession, dawnDevice);

	return XR_SUCCESS;
}

} // namespace dawnxr::internal
