#include "core.h"
#include "VulkanRenderer.h"
#include "Log.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

VulkanRenderer::VulkanRenderer()
{
#ifdef NDEBUG
	m_bEnableValidationLayer = false;
#else
	m_bEnableValidationLayer = true;
#endif

	m_uiWindowWidth = 800;
	m_uiWindowHeight = 600;
	m_strWindowTitle = "Vulkan Renderer";

	m_mapShaderPath = {
		{ VK_SHADER_STAGE_VERTEX_BIT,	"./Assert/Shader/vert.spv" },
		{ VK_SHADER_STAGE_FRAGMENT_BIT,	"./Assert/Shader/frag.spv" },
	};
}

VulkanRenderer::~VulkanRenderer()
{
	Clean();
}

void VulkanRenderer::Init()
{
	InitWindow();
	CreateInstance();
	CreateWindowSurface();
	PickBestPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateDescriptorPool();
}

void VulkanRenderer::Loop()
{
}

void VulkanRenderer::Clean()
{
}

void VulkanRenderer::LoadModel(const std::filesystem::path& modelPath)
{
}

void VulkanRenderer::FrameBufferResizeCallBack(GLFWwindow* pWindow, int nWidth, int nHeight)
{
	auto vulkanRenderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(pWindow));
	if (vulkanRenderer)
		vulkanRenderer->m_bFrameBufferResized = true;
}

void VulkanRenderer::InitWindow()
{
	glfwInit();
	ASSERT(glfwVulkanSupported(), "GLFW version not support vulkan");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_pWindow = glfwCreateWindow(m_uiWindowWidth, m_uiWindowHeight, m_strWindowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(m_pWindow);

	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetFramebufferSizeCallback(m_pWindow, FrameBufferResizeCallBack);
}

void VulkanRenderer::QueryGLFWExtensions()
{
	const char** ppGLFWEntensions = nullptr;
	UINT uiCount = 0;
	ppGLFWEntensions = glfwGetRequiredInstanceExtensions(&uiCount);

	m_vecChosedExtensions = std::vector<const char*>(ppGLFWEntensions, ppGLFWEntensions + uiCount);
}

void VulkanRenderer::QueryValidationLayerExtensions()
{
	m_vecChosedExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

void VulkanRenderer::QueryAllValidExtensions()
{
	UINT uiExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, nullptr);

	ASSERT(uiExtensionCount > 0, "Find no valid extensions");

	m_vecValidExtensions.resize(uiExtensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &uiExtensionCount, m_vecValidExtensions.data());
}

bool VulkanRenderer::IsExtensionValid(const std::string& strExtensionName)
{
	for (const auto& extensionProperty : m_vecValidExtensions)
	{
		if (extensionProperty.extensionName == strExtensionName)
			return true;
	}
	return false;
}

bool VulkanRenderer::CheckChosedExtensionValid()
{
	QueryGLFWExtensions();
	QueryValidationLayerExtensions();

	QueryAllValidExtensions();

	for (const auto& extensionName : m_vecChosedExtensions)
	{
		if (!IsExtensionValid(extensionName))
			return false;
	}
	return true;
}

void VulkanRenderer::QueryValidationLayers()
{
	m_vecChosedValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
}

void VulkanRenderer::QueryAllValidValidationLayers()
{
	uint32_t uiLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&uiLayerCount, nullptr);

	ASSERT(uiLayerCount > 0, "Find no valid validation layer");

	m_vecValidValidationLayers.resize(uiLayerCount);
	vkEnumerateInstanceLayerProperties(&uiLayerCount, m_vecValidValidationLayers.data());
}

bool VulkanRenderer::IsValidationLayerValid(const std::string& strValidationLayerName)
{
	for (const auto& layerProperty : m_vecValidValidationLayers)
	{
		if (layerProperty.layerName == strValidationLayerName)
			return true;
	}
	return false;
}

bool VulkanRenderer::CheckChosedValidationLayerValid()
{
	QueryValidationLayers();

	QueryAllValidValidationLayers();

	for (const auto& layerName : m_vecChosedValidationLayers)
	{
		if (!IsValidationLayerValid(layerName))
			return false;
	}
	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT messageServerity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	Log::Critical(std::format("Validation Layer Debug Callback : {}", pCallbackData->pMessage));
	return VK_FALSE;
}

VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//属于Extension的函数，不能直接使用，需要先封装一层，使用vkGetInstanceProcAddr判断该函数是否可用
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanRenderer::DestoryDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	//属于Extension的函数，不能直接使用，需要先封装一层，使用vkGetInstanceProcAddr判断该函数是否可用
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func)
		func(instance, debugMessenger, pAllocator);
}

void VulkanRenderer::FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo)
{
	messengerCreateInfo = {};
	messengerCreateInfo.sType =
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;/* |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;*/
	messengerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	messengerCreateInfo.pfnUserCallback = debugCallBack;
}

void VulkanRenderer::SetupDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo)
{
	VULKAN_ASSERT(CreateDebugUtilsMessengerEXT(m_Instance, &messengerCreateInfo, nullptr, &m_DebugMessenger), "Setup debug messenger failed");
}

void VulkanRenderer::CreateInstance()
{
	CheckChosedExtensionValid();
	if (m_bEnableValidationLayer)
		CheckChosedValidationLayerValid();

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_strWindowTitle.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
	appInfo.pEngineName = nullptr;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<UINT>(m_vecChosedExtensions.size());
	createInfo.ppEnabledExtensionNames = m_vecChosedExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};

	if (m_bEnableValidationLayer)
	{
		createInfo.enabledLayerCount = static_cast<int>(m_vecChosedValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_vecChosedValidationLayers.data();
		
		FillDebugMessengerCreateInfo(debugMessengerCreateInfo);
		createInfo.pNext = &debugMessengerCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;
	}

	VULKAN_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_Instance), "Create instance failed");

	if (m_bEnableValidationLayer)
		SetupDebugMessenger(debugMessengerCreateInfo);
}

void VulkanRenderer::CreateWindowSurface()
{
	VULKAN_ASSERT(glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_WindowSurface), "Create window surface failed");
}

void VulkanRenderer::QueryAllValidPhysicalDevice()
{
	UINT uiPhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &uiPhysicalDeviceCount, nullptr);
	
	ASSERT(uiPhysicalDeviceCount > 0, "Find no valid physical device");

	m_vecValidPhysicalDevices.resize(uiPhysicalDeviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &uiPhysicalDeviceCount, m_vecValidPhysicalDevices.data());

	for (const auto& physicalDevice : m_vecValidPhysicalDevices)
	{
		PhysicalDeviceInfo info;
		
		vkGetPhysicalDeviceProperties(physicalDevice, &info.properties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &info.features);

		UINT uiQueueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &uiQueueFamilyCount, nullptr);
		if (uiQueueFamilyCount > 0)
		{
			info.vecQueueFamilies.resize(uiQueueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &uiQueueFamilyCount, info.vecQueueFamilies.data());
		}

		int nIdx = 0;
		for (const auto& queueFamily : info.vecQueueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				info.graphicFamilyIdx = nIdx;
				break;
			}
			nIdx++;
		}

		nIdx = 0;
		VkBool32 bPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, nIdx, m_WindowSurface, &bPresentSupport);
		if (bPresentSupport)
			info.presentFamilyIdx = nIdx;

		//获取硬件支持的capability，包含Image数量上下限等信息
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_WindowSurface, &info.swapChainSupportInfo.capabilities);

		//获取硬件支持的Surface Format列表
		UINT uiFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_WindowSurface, &uiFormatCount, nullptr);
		if (uiFormatCount > 0)
		{
			info.swapChainSupportInfo.vecSurfaceFormats.resize(uiFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_WindowSurface, &uiFormatCount, info.swapChainSupportInfo.vecSurfaceFormats.data());
		}

		//获取硬件支持的Present Mode列表
		UINT uiPresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_WindowSurface, &uiPresentModeCount, nullptr);
		if (uiPresentModeCount > 0)
		{
			info.swapChainSupportInfo.vecPresentModes.resize(uiPresentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_WindowSurface, &uiPresentModeCount, info.swapChainSupportInfo.vecPresentModes.data());
		}

		UINT uiExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &uiExtensionCount, nullptr);
		if (uiExtensionCount > 0)
		{
			info.vecAvaliableDeviceExtensions.resize(uiExtensionCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &uiExtensionCount, info.vecAvaliableDeviceExtensions.data());
		}

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &info.memoryProperties);

		info.nRateScore = RatePhysicalDevice(info);

		m_mapPhysicalDeviceInfo[physicalDevice] = info;
	}
}

int VulkanRenderer::RatePhysicalDevice(PhysicalDeviceInfo& deviceInfo)
{
	int nScore = 0;

	//检查是否是独立显卡
	if (deviceInfo.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		nScore += 1000;

	//检查支持的最大图像尺寸,越大越好
	nScore += deviceInfo.properties.limits.maxImageDimension2D;

	//检查是否支持几何着色器
	if (!deviceInfo.features.geometryShader)
		return 0;

	//检查是否支持Graphic Family, Present Family且Index一致
	if (!deviceInfo.IsGraphicAndPresentQueueFamilySame())
		return 0;

	//检查是否支持指定的Extension(Swap Chain等)
	bool bExtensionSupport = checkDeviceExtensionSupport(deviceInfo);
	if (!bExtensionSupport)
		return 0;

	////检查Swap Chain是否满足要求
	//bool bSwapChainAdequate = false;
	//if (bExtensionSupport) //首先确认支持SwapChain
	//{
	//	SwapChainSupportDetails details = querySwapChainSupport(device);
	//	bSwapChainAdequate = !details.vecSurfaceFormats.empty() && !details.vecPresentModes.empty();
	//}
	//if (!bSwapChainAdequate)
	//	return false;

	////检查是否支持各向异性过滤
	//if (!deviceFeatures.samplerAnisotropy)
	//	return false;

	return nScore;
}

void VulkanRenderer::PickBestPhysicalDevice()
{
	QueryAllValidPhysicalDevice();

	VkPhysicalDevice bestDevice;
	int nScore = 0;

	for (const auto& iter : m_mapPhysicalDeviceInfo)
	{
		if (iter.second.nRateScore > nScore)
		{
			nScore = iter.second.nRateScore;
			bestDevice = iter.first;
		}
	}

	m_PhysicalDevice = bestDevice;
}

bool VulkanRenderer::checkDeviceExtensionSupport(const PhysicalDeviceInfo& deviceInfo)
{
	std::set<std::string> setRequiredExtensions(m_vecDeviceExtensions.begin(), m_vecDeviceExtensions.end());

	for (const auto& extension : deviceInfo.vecAvaliableDeviceExtensions)
	{
		setRequiredExtensions.erase(extension.extensionName);
	}

	return setRequiredExtensions.empty();
}

void VulkanRenderer::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> vecQueueCreateInfo;

	auto iter = m_mapPhysicalDeviceInfo.find(m_PhysicalDevice);
	ASSERT((iter != m_mapPhysicalDeviceInfo.end()), "Cant find physical device info");

	const auto& physicalDeviceInfo = iter->second;
	float queuePriority = 1.f;

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = physicalDeviceInfo.graphicFamilyIdx.value();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority; //Queue的优先级，范围[0.f, 1.f]，控制CommandBuffer的执行顺序
	vecQueueCreateInfo.push_back(queueCreateInfo);

	VkPhysicalDeviceFeatures deviceFeatures{};
	//deviceFeatures.samplerAnisotropy = VK_TRUE; //启用各向异性过滤，用于纹理采样
	//deviceFeatures.sampleRateShading = VK_TRUE;	//启用Sample Rate Shaing，用于MSAA抗锯齿

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<UINT>(vecQueueCreateInfo.size());
	createInfo.pQueueCreateInfos = vecQueueCreateInfo.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<UINT>(m_vecDeviceExtensions.size()); //注意！此处的extension与创建Instance时不同
	createInfo.ppEnabledExtensionNames = m_vecDeviceExtensions.data();
	if (m_bEnableValidationLayer)
	{
		createInfo.enabledLayerCount = static_cast<UINT>(m_vecChosedValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_vecChosedValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	VULKAN_ASSERT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice), "Create logical device failed");

	vkGetDeviceQueue(m_LogicalDevice, physicalDeviceInfo.graphicFamilyIdx.value(), 0, &m_GraphicQueue);
	vkGetDeviceQueue(m_LogicalDevice, physicalDeviceInfo.presentFamilyIdx.value(), 0, &m_PresentQueue);
}

VkSurfaceFormatKHR VulkanRenderer::ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vecAvailableFormats)
{
	ASSERT(vecAvailableFormats.size() > 0, "No avaliable swap chain surface format");

	//找到支持sRGB格式的format，如果没有则返回第一个
	for (const auto& format : vecAvailableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return vecAvailableFormats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& vecAvailableModes)
{
	ASSERT(vecAvailableModes.size() > 0, "No avaliable swap chain present mode");

	//优先选择MAILBOX，其次FIFO
	for (const auto& mode : vecAvailableModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapChainSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	int nWidth = 0;
	int nHeight = 0;
	glfwGetFramebufferSize(m_pWindow, &nWidth, &nHeight);

	VkExtent2D actualExtent = { static_cast<UINT>(nWidth), static_cast<UINT>(nHeight) };

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

UINT VulkanRenderer::ChooseSwapChainImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
	uint32_t uiImageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0)
	{
		uiImageCount = std::clamp(uiImageCount, capabilities.minImageCount, capabilities.maxImageCount);
	}
	return uiImageCount;
}

void VulkanRenderer::CreateSwapChain()
{
	auto iter = m_mapPhysicalDeviceInfo.find(m_PhysicalDevice);

	ASSERT((iter != m_mapPhysicalDeviceInfo.end()), "Cant find physical device info");

	const auto& physicalDeviceInfo = iter->second;

	m_SwapChainSurfaceFormat = ChooseSwapChainSurfaceFormat(physicalDeviceInfo.swapChainSupportInfo.vecSurfaceFormats);
	m_SwapChainFormat = m_SwapChainSurfaceFormat.format;
	m_SwapChainPresentMode = ChooseSwapChainPresentMode(physicalDeviceInfo.swapChainSupportInfo.vecPresentModes);
	m_SwapChainExtent2D = ChooseSwapChainSwapExtent(physicalDeviceInfo.swapChainSupportInfo.capabilities);
	UINT uiImageCount = ChooseSwapChainImageCount(physicalDeviceInfo.swapChainSupportInfo.capabilities);

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_WindowSurface;
	createInfo.minImageCount = uiImageCount;
	createInfo.imageFormat = m_SwapChainFormat;
	createInfo.imageColorSpace = m_SwapChainSurfaceFormat.colorSpace;
	createInfo.imageExtent = m_SwapChainExtent2D;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.presentMode = m_SwapChainPresentMode;

	//挑选显卡时已确保GraphicFamily与PresentFamily一致
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;

	//设置如何进行Transform
	createInfo.preTransform = physicalDeviceInfo.swapChainSupportInfo.capabilities.currentTransform; //不做任何Transform

	//设置是否启用Alpha通道
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //不启用Alpha通道

	//设置是否启用隐藏面剔除
	createInfo.clipped = VK_TRUE;

	//oldSwapChain用于window resize时
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VULKAN_ASSERT(vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_SwapChain), "Create swap chain failed");
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, UINT uiMipLevel)
{
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = uiMipLevel;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VULKAN_ASSERT(vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &imageView), "Create image view failed");

	return imageView;
}

void VulkanRenderer::CreateSwapChainImages()
{
	UINT uiImageCount = 0;
	vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &uiImageCount, nullptr);
	ASSERT(uiImageCount > 0, "Find no image in swap chain");
	m_vecSwapChainImages.resize(uiImageCount);
	vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &uiImageCount, m_vecSwapChainImages.data());
}

void VulkanRenderer::CreateSwapChainImageViews()
{
	m_vecSwapChainImageViews.resize(m_vecSwapChainImages.size());
	for (UINT i = 0; i < m_vecSwapChainImageViews.size(); ++i)
	{
		m_vecSwapChainImageViews[i] = CreateImageView(m_vecSwapChainImages[i], m_SwapChainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

VkFormat VulkanRenderer::ChooseDepthFormat(const VkImageTiling& tiling, const VkFormatFeatureFlags& feature)
{
	//后两种格式包含Stencil Component
	std::vector<VkFormat> vecFormats = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM,
	};

	for (const auto& format : vecFormats)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR) && ((properties.linearTilingFeatures & feature) == feature))
		{
			return format;
		}
		else if ((tiling == VK_IMAGE_TILING_OPTIMAL) && ((properties.optimalTilingFeatures & feature) == feature))
		{
			return format;
		}
	}

	ASSERT(false, "No support depth format");
	return VK_FORMAT_D32_SFLOAT_S8_UINT;
}

void VulkanRenderer::CreateRenderPass()
{
	std::array<VkAttachmentDescription, 2> attachmentDescriptions = {};

	//设置Color Attachment Description, Reference
	attachmentDescriptions[0].format = m_SwapChainFormat;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT; //不使用多重采样
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //RenderPass开始前清除
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; //RenderPass结束后保留其内容用来present
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //初始布局不重要
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //适用于present的布局

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//设置Depth Attachment Description, Reference
	attachmentDescriptions[1].format = ChooseDepthFormat(VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT; //不使用多重采样
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //RenderPass开始前清除
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //RenderPass结束后不再需要
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; //适用于depth stencil的布局

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;
	subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkSubpassDependency, 2> dependencies = {};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	dependencies[0].dependencyFlags = 0;

	dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstSubpass = 0;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = 0;
	dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	dependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<UINT>(attachmentDescriptions.size());
	renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = static_cast<UINT>(dependencies.size());
	renderPassCreateInfo.pDependencies = dependencies.data();

	VULKAN_ASSERT(vkCreateRenderPass(m_LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass), "Create render pass failed");
}

std::vector<char> VulkanRenderer::ReadShaderFile(const std::filesystem::path& filepath)
{
	ASSERT(std::filesystem::exists(filepath), std::format("Shader file path {} not exist", filepath.string()));
	//光标置于文件末尾，方便统计长度
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	ASSERT(file.is_open(), std::format("Open shader file {} failed", filepath.string()));

	//tellg获取当前文件读写位置
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> vecBuffer(fileSize);

	//指针回到文件开头
	file.seekg(0);
	file.read(vecBuffer.data(), fileSize);

	file.close();
	return vecBuffer;
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& vecBytecode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = vecBytecode.size();
	createInfo.pCode = reinterpret_cast<const UINT*>(vecBytecode.data());

	VkShaderModule shaderModule;
	VULKAN_ASSERT(vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule), "Create shader module failed");

	return shaderModule;
}

void VulkanRenderer::CreateShader()
{
	m_mapShaderModule.clear();

	ASSERT(m_mapShaderPath.size() > 0, "Detect no shader spv file");

	for (const auto& spvPath : m_mapShaderPath)
	{
		auto shaderModule = CreateShaderModule(ReadShaderFile(spvPath.second));

		m_mapShaderModule[spvPath.first] = shaderModule;
	}
}

UINT VulkanRenderer::FindSuitableMemoryTypeIndex(UINT typeFilter, VkMemoryPropertyFlags properties)
{
	auto iter = m_mapPhysicalDeviceInfo.find(m_PhysicalDevice);
	ASSERT(iter != m_mapPhysicalDeviceInfo.end(), "Find no physical device info");

	const auto& memoryProperties = iter->second.memoryProperties;

	for (UINT i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if (typeFilter & (1 << i))
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}
	}

	ASSERT(false, "Find no suitable memory type");
}

void VulkanRenderer::CreateBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo BufferCreateInfo{};
	BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.size = deviceSize;	//要创建的Buffer的大小
	BufferCreateInfo.usage = usageFlags;	//使用目的，比如用作VertexBuffer或IndexBuffer或其他
	BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //Buffer可以被多个QueueFamily共享，这里选择独占模式
	BufferCreateInfo.flags = 0;	//用来配置缓存的内存稀疏程度，0为默认值

	VULKAN_ASSERT(vkCreateBuffer(m_LogicalDevice, &BufferCreateInfo, nullptr, &buffer), "Create buffer failed");

	//MemoryRequirements的参数如下：
	//memoryRequirements.size			所需内存的大小
	//memoryRequirements.alignment		所需内存的对齐方式，由Buffer的usage和flags参数决定
	//memoryRequirements.memoryTypeBits 适合该Buffer的内存类型（位值）

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memoryRequirements);
	
	VkMemoryAllocateInfo memoryAllocInfo{};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	//显卡可以不同类型的内存，不同类型的内存所允许的操作与效率各不相同，需要根据需求寻找最适合的内存类型
	memoryAllocInfo.memoryTypeIndex = FindSuitableMemoryTypeIndex(memoryRequirements.memoryTypeBits, propertyFlags);

	VULKAN_ASSERT(vkAllocateMemory(m_LogicalDevice, &memoryAllocInfo, nullptr, &bufferMemory), "Allocate buffer memory failed");

	vkBindBufferMemory(m_LogicalDevice, buffer, bufferMemory, 0);
}

void VulkanRenderer::CreateUniformBuffers()
{
	VkDeviceSize uniformBufferSize = sizeof(UniformBufferObject);

	m_vecUniformBuffers.resize(m_vecSwapChainImages.size());
	m_vecUniformBufferMemories.resize(m_vecSwapChainImages.size());

	//为并行渲染的每一帧图像创建独立的Uniform Buffer
	for (size_t i = 0; i < m_vecSwapChainImages.size(); ++i)
	{
		CreateBuffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_vecUniformBuffers[i], m_vecUniformBufferMemories[i]);
	}
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
	//UniformBufferObject Binding
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0; //对应Vertex Shader中的layout(binding=0)
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	//类型为Uniform Buffer
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //只需要在vertex stage生效
	uboLayoutBinding.pImmutableSamplers = nullptr;

	//CombinedImageSampler Binding
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1; ////对应Fragment Shader中的layout(binding=1)
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //只用于fragment stage
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> vecDescriptorLayoutBinding = {
		uboLayoutBinding,
		samplerLayoutBinding,
	};

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<UINT>(vecDescriptorLayoutBinding.size());
	createInfo.pBindings = vecDescriptorLayoutBinding.data();

	VULKAN_ASSERT(vkCreateDescriptorSetLayout(m_LogicalDevice, &createInfo, nullptr, &m_DescriptorSetLayout), "Create descriptor layout failed");
}

void VulkanRenderer::CreateDescriptorPool()
{
	//ubo
	VkDescriptorPoolSize uboPoolSize{};
	uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboPoolSize.descriptorCount = static_cast<UINT>(m_vecSwapChainImages.size());

	//sampler
	VkDescriptorPoolSize samplerPoolSize{};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = static_cast<UINT>(m_vecSwapChainImages.size());

	std::vector<VkDescriptorPoolSize> vecPoolSize = {
		uboPoolSize,
		samplerPoolSize,
	};

	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = static_cast<UINT>(vecPoolSize.size());
	poolCreateInfo.pPoolSizes = vecPoolSize.data();
	poolCreateInfo.maxSets = static_cast<UINT>(m_vecSwapChainImages.size());

	VULKAN_ASSERT(vkCreateDescriptorPool(m_LogicalDevice, &poolCreateInfo, nullptr, &m_DescriptorPool), "Create descriptor pool failed");
}

void VulkanRenderer::CreateDescriptorSets()
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorSetCount = static_cast<UINT>(m_vecSwapChainImages.size());
	allocInfo.descriptorPool = m_DescriptorPool;

	std::vector<VkDescriptorSetLayout> vecDupDescriptorSetLayout(m_vecSwapChainImages.size(), m_DescriptorSetLayout);
	allocInfo.pSetLayouts = vecDupDescriptorSetLayout.data();

	m_vecDescriptorSets.resize(m_vecSwapChainImages.size());
	VkResult res = vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, m_vecDescriptorSets.data());
	if (res != VK_SUCCESS)
		throw std::runtime_error("Allocate Descriptor Sets Failed");

	for (size_t i = 0; i < m_vecSwapChainImages.size(); ++i)
	{
		//ubo
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_vecUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet uboWrite{};
		uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uboWrite.dstSet = m_vecDescriptorSets[i];
		uboWrite.dstBinding = 0;
		uboWrite.dstArrayElement = 0;
		uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboWrite.descriptorCount = 1;
		uboWrite.pBufferInfo = &bufferInfo;

		//sampler
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_TextureImageView;
		imageInfo.sampler = m_TextureSampler;

		VkWriteDescriptorSet samplerWrite{};
		samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		samplerWrite.dstSet = m_vecDescriptorSets[i];
		samplerWrite.dstBinding = 1;
		samplerWrite.dstArrayElement = 0;
		samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerWrite.descriptorCount = 1;
		samplerWrite.pImageInfo = &imageInfo;

		std::vector<VkWriteDescriptorSet> vecDescriptorWrite = {
			uboWrite,
			samplerWrite,
		};

		vkUpdateDescriptorSets(m_LogicalDevice, static_cast<uint32_t>(vecDescriptorWrite.size()), vecDescriptorWrite.data(), 0, nullptr);
	}
}



