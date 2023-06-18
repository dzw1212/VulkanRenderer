#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();


	void Init();
	void Loop();
	void Clean();

private:
	static void FrameBufferResizeCallBack(GLFWwindow* pWindow, int nWidth, int nHeight);
	void InitWindow();

	void QueryGLFWExtensions();
	void QueryValidationLayerExtensions();
	void QueryAllValidExtensions();
	bool IsExtensionValid(const std::string& strExtensionName);
	bool CheckChosedExtensionValid();

	void QueryValidationLayers();
	void QueryAllValidValidationLayers();
	bool IsValidationLayerValid(const std::string& strValidationLayerName);
	bool CheckChosedValidationLayerValid();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageServerity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestoryDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);
	void FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo);
	void SetupDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo);

	void CreateInstance();

	void CreateWindowSurface();


	struct SwapChainSupportInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;	//SwapChain容量相关信息
		std::vector<VkSurfaceFormatKHR> vecSurfaceFormats;	//支持哪些图像格式
		std::vector<VkPresentModeKHR> vecPresentModes;	//支持哪些表现模式，如二缓、三缓等
	};

	struct PhysicalDeviceInfo
	{
		PhysicalDeviceInfo()
		{
			nRateScore = 0;
			graphicFamilyIdx = std::nullopt;
			presentFamilyIdx = std::nullopt;
		}

		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		std::vector<VkQueueFamilyProperties> vecQueueFamilies;

		std::vector<VkExtensionProperties> vecAvaliableDeviceExtensions;

		int nRateScore;

		std::optional<UINT> graphicFamilyIdx;
		std::optional<UINT> presentFamilyIdx;

		bool HaveGraphicAndPresentQueueFamily()
		{
			return graphicFamilyIdx.has_value() && presentFamilyIdx.has_value();
		}

		bool IsGraphicAndPresentQueueFamilySame()
		{
			return HaveGraphicAndPresentQueueFamily() && (graphicFamilyIdx == presentFamilyIdx);
		}

		SwapChainSupportInfo swapChainSupportInfo;
	};
	void QueryAllValidPhysicalDevice();
	int RatePhysicalDevice(PhysicalDeviceInfo& deviceInfo);
	void PickBestPhysicalDevice();

	bool checkDeviceExtensionSupport(const PhysicalDeviceInfo& deviceInfo);
	void CreateLogicalDevice();


	VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vecAvailableFormats);
	VkPresentModeKHR ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& vecAvailableModes);
	VkExtent2D ChooseSwapChainSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	UINT ChooseSwapChainImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
	void CreateSwapChain();


	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, UINT uiMipLevel);
	void CreateSwapChainImageViews();

	VkFormat ChooseDepthFormat(const VkImageTiling& tiling, const VkFormatFeatureFlags& feature);
	void CreateRenderPass();


	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();

private:
	UINT m_uiWindowWidth;
	UINT m_uiWindowHeight;
	std::string m_strWindowTitle;
	bool m_bFrameBufferResized;
	
	GLFWwindow* m_pWindow;

	std::vector<const char*> m_vecChosedExtensions;
	std::vector<VkExtensionProperties> m_vecValidExtensions;

	bool m_bEnableValidationLayer;
	std::vector<const char*> m_vecChosedValidationLayers;
	std::vector<VkLayerProperties> m_vecValidValidationLayers;

	VkDebugUtilsMessengerEXT m_DebugMessenger;

	VkInstance m_Instance;

	VkSurfaceKHR m_WindowSurface;

	std::vector<VkPhysicalDevice> m_vecValidPhysicalDevices;
	std::unordered_map<VkPhysicalDevice, PhysicalDeviceInfo> m_mapPhysicalDeviceInfo;
	VkPhysicalDevice m_PhysicalDevice;

	VkDevice m_LogicalDevice;
	const std::vector<const char*> m_vecDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	VkQueue m_GraphicQueue;
	VkQueue m_PresentQueue;

	VkSwapchainKHR m_SwapChain;
	VkSurfaceFormatKHR m_SwapChainSurfaceFormat;
	VkFormat m_SwapChainFormat;
	VkPresentModeKHR m_SwapChainPresentMode;
	VkExtent2D m_SwapChainExtent2D;

	std::vector<VkImage> m_vecSwapChainImages;
	std::vector<VkImageView> m_vecSwapChainImageViews;

	VkRenderPass m_RenderPass;

	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSet m_DescriptorSet;

};