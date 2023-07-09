#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../Core.h"

class VulkanRenderer;

class UI
{
public:
	UI() = default;

	void Init(VulkanRenderer& renderer);
	VkCommandBuffer& FillCommandBuffer(VulkanRenderer& renderer, UINT uiIdx);
	void Render_Begin();
	void Render_End();
	void Clean();

	void testSameRenderPass(VkCommandBuffer& commandBuffer);
};