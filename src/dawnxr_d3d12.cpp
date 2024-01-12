#include "dawnxr_internal.h"

#include <dawn/native/D3D12Backend.h>

#include <iostream>
#include <vector>

#include "d3dx/d3dx12_core.h"

using namespace dawnxr::internal;

namespace {

const auto dawnSwapchainFormat = wgpu::TextureFormat::BGRA8UnormSrgb;
const auto d3d12SwapchainFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

struct D3D12Session : Session {

	D3D12Session(XrSession session, const wgpu::Device& device) : Session(session, device) {
	}

	XrResult enumerateSwapchainFormats(std::vector<wgpu::TextureFormat>& formats) override {

		formats.push_back(dawnSwapchainFormat);

		return XR_SUCCESS;
	}

	XrResult createSwapchain(const XrSwapchainCreateInfo* createInfo, std::vector<wgpu::Texture>& images,
							 XrSwapchain* swapchain) override {

		if (createInfo->type != XR_TYPE_SWAPCHAIN_CREATE_INFO) return XR_ERROR_HANDLE_INVALID;

		if (createInfo->format != (int64_t)dawnSwapchainFormat) return XR_ERROR_RUNTIME_FAILURE;

		auto d3d12Info = *createInfo;
		d3d12Info.format = d3d12SwapchainFormat;

		if (0) {	// NOLINT
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc{};
			textureDesc.MipLevels = d3d12Info.mipCount;
			textureDesc.Format = (DXGI_FORMAT)d3d12Info.format;
			textureDesc.Width = d3d12Info.width;
			textureDesc.Height = d3d12Info.height;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			textureDesc.DepthOrArraySize = d3d12Info.arraySize;
			textureDesc.SampleDesc.Count = d3d12Info.sampleCount;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			auto d3d12Device = dawn::native::d3d12::GetD3D12Device(device.Get()).Get();

			CD3DX12_HEAP_PROPERTIES heapProperties{D3D12_HEAP_TYPE_DEFAULT};

			ID3D12Resource* resource;

			std::cout << "### D3D12 Create Texture: "
					  << d3d12Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
															  D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
															  IID_PPV_ARGS(&resource))
					  << std::endl;
		}

		XR_TRY(xrCreateSwapchain(backendSession, &d3d12Info, swapchain));
		// TODO: Need to cleanup swapchain if any of the below fails

		uint32_t n;
		XR_TRY(xrEnumerateSwapchainImages(*swapchain, 0, &n, nullptr));

		std::vector<XrSwapchainImageD3D12KHR> d3d12Images(n, {XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
		XR_TRY(xrEnumerateSwapchainImages(*swapchain, n, &n, (XrSwapchainImageBaseHeader*)d3d12Images.data()));
		if (n != d3d12Images.size()) return XR_ERROR_RUNTIME_FAILURE;

		wgpu::TextureDescriptor textureDesc{
			nullptr,												  // nextInChain
			nullptr,												  // label
			wgpu::TextureUsage::RenderAttachment |					  // usage
				wgpu::TextureUsage::TextureBinding,					  // ...does this need to be optional?
			wgpu::TextureDimension::e2D,							  // dimension
			wgpu::Extent3D{createInfo->width, createInfo->height, 1}, // size
			(wgpu::TextureFormat)createInfo->format,				  // format
			createInfo->mipCount,									  // mipLevelCount;
			createInfo->sampleCount,								  // sampleCount;
			0,														  // viewFormatCount;
			nullptr													  // view formats
		};

		for (auto& it : d3d12Images) {
			auto texture = wgpu::Texture(dawn::native::d3d12::CreateSwapchainWGPUTexture(
				device.Get(), (WGPUTextureDescriptor*)&textureDesc, it.texture));
			images.push_back(texture);
		}

		return XR_SUCCESS;
	}
};

} // namespace

namespace dawnxr::internal {

XrResult getD3D12GraphicsRequirements(XrInstance instance, XrSystemId systemId, GraphicsRequirementsDawn* requirements) {

	PFN_xrGetD3D12GraphicsRequirementsKHR xrGetD3D12GraphicsRequirementsKHR = nullptr;
	XR_TRY(xrGetInstanceProcAddr(instance, "xrGetD3D12GraphicsRequirementsKHR",
								 (PFN_xrVoidFunction*)(&xrGetD3D12GraphicsRequirementsKHR)));

	XrGraphicsRequirementsD3D12KHR d3d12Reqs{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
	XR_TRY(xrGetD3D12GraphicsRequirementsKHR(instance, systemId, &d3d12Reqs));

//	std::cout << "### D3D12 graphics requirements minFeatureLevel: " << d3d12Reqs.minFeatureLevel << std::endl;
//	std::cout << "### D3D12 graphics requirements adapterLuid: " << d3d12Reqs.adapterLuid.HighPart << " "
//			  << d3d12Reqs.adapterLuid.LowPart << std::endl;

	return XR_SUCCESS;
}

XrResult createD3D12RequestAdapterOptions(XrInstance instance, XrSystemId systemId, wgpu::ChainedStruct** opts) {

	// TODO: Should really add a D3D_FEATURE_LEVEL adapter option to dawn, as it's hardcoded to 11_0 in dawn but OpenXR wants 12_0.

	PFN_xrGetD3D12GraphicsRequirementsKHR xrGetD3D12GraphicsRequirementsKHR = nullptr;
	XR_TRY(xrGetInstanceProcAddr(instance, "xrGetD3D12GraphicsRequirementsKHR",
								 (PFN_xrVoidFunction*)(&xrGetD3D12GraphicsRequirementsKHR)));

	XrGraphicsRequirementsD3D12KHR d3d12Reqs{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
	XR_TRY(xrGetD3D12GraphicsRequirementsKHR(instance, systemId, &d3d12Reqs));

	auto adapterOpts = new dawn::native::d3d::RequestAdapterOptionsLUID();
	adapterOpts->adapterLUID = d3d12Reqs.adapterLuid;
	*opts = adapterOpts;

	return XR_SUCCESS;
}

XrResult createD3D12Session(XrInstance instance, const XrSessionCreateInfo* createInfo, Session** session) {

	if (createInfo->type != XR_TYPE_SESSION_CREATE_INFO) return XR_ERROR_HANDLE_INVALID;

	auto dawnBinding = (GraphicsBindingDawn*)createInfo->next;
	if (dawnBinding->type != XR_TYPE_GRAPHICS_BINDING_DAWN_EXT) return XR_ERROR_HANDLE_INVALID;
	auto dawnDevice = dawnBinding->device;

	XrGraphicsBindingD3D12KHR d3d12Binding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};

	d3d12Binding.device = dawn::native::d3d12::GetD3D12Device(dawnDevice.Get()).Get();
	d3d12Binding.queue = dawn::native::d3d12::GetD3D12CommandQueue(dawnDevice.Get()).Get();

//	auto luid = d3d12Binding.device->GetAdapterLuid();
//	std::cout << "### D3D12 Device adapter LUID: " << luid.HighPart << " " << luid.LowPart << std::endl;

	XrSessionCreateInfo d3d12CreateInfo{XR_TYPE_SESSION_CREATE_INFO};
	d3d12CreateInfo.next = &d3d12Binding;
	d3d12CreateInfo.systemId = createInfo->systemId;

	XrSession backendSession;
	XR_TRY(xrCreateSession(instance, &d3d12CreateInfo, &backendSession));
	*session = new D3D12Session(backendSession, dawnDevice);

	return XR_SUCCESS;
}

} // namespace dawnxr::internal
