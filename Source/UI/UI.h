#pragma once

struct VulkanRenderer;
struct VkCommandBuffer;

class UI
{
public:
	UI() = default;

	void Init(const VulkanRenderer& renderer);
	void Render(VkCommandBuffer& commandBuffer);
	void Present();
	void Clean();

private:
	VulkanRenderer m_Renderer;
};