#pragma once

struct VulkanRenderer;

class UI
{
public:
	UI();
	~UI();

	void Init(VulkanRenderer& renderer);
	void Tick();
	void Clean();

private:

};