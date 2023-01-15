Simple dawn / openxr interop library

(Currently only handles d3d12!)

I've tried to do it so dawn can be treated like just another openxr platform, so the only additions are:

typedef struct XrGraphicsBindingDawnEXT {
	XrStructureType type;
	const void* XR_MAY_ALIAS next;
	WGPUDevice device;
} XrGraphicsBindingDawnEXT;

typedef struct XrSwapchainImageDawnEXT {
	XrStructureType type;
	void* XR_MAY_ALIAS next;
	WGPUTextureView textureView;
} XrSwapchainImageDawnEXT;

typedef struct XrGraphicsRequirementsDawnEXT {
	XrStructureType type;
	void* XR_MAY_ALIAS next;
	//WGPUAdapterDescriptor
} XrGraphicsRequirementsDawnEXT;

XrResult xrGetDawnGraphicsRequirementsEXT(XrInstance instance, XrSystemId systemId, WGPUBackendType backendType,
										  XrGraphicsRequirementsDawnEXT* graphicsRequirements);


