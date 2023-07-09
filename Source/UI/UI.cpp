#include "UI.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort

#include "../VulkanRenderer.h"

static VkDescriptorPool             g_DescriptorPool            = VK_NULL_HANDLE;
static VkRenderPass                 g_RenderPass                = VK_NULL_HANDLE;
static VkCommandPool                g_CommandPool               = VK_NULL_HANDLE;
static std::vector<VkCommandBuffer> g_vecCommandBuffers;
static std::vector<VkFramebuffer>   g_vecFrameBuffers;

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void UI::Init(VulkanRenderer& renderer)
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    VULKAN_ASSERT(vkCreateDescriptorPool(renderer.GetLogicalDevice(), &pool_info, nullptr, &g_DescriptorPool), "Create ImGui descriptor pool failed");

    VkAttachmentDescription attachment = {};
    attachment.format = renderer.GetUISwapChainFormat();
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;
    VULKAN_ASSERT(vkCreateRenderPass(renderer.GetLogicalDevice(), &info, nullptr, &g_RenderPass), "Create ImGui render pass failed");

    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = renderer.GetGraphicQueueIdx();
    VULKAN_ASSERT(vkCreateCommandPool(renderer.GetLogicalDevice(), &commandPoolCreateInfo, nullptr, &g_CommandPool), "Create ImGui command pool failed");

    g_vecCommandBuffers.resize(renderer.GetSwapChainImageCount());
    VkCommandBufferAllocateInfo commandBufferAllocator{};
    commandBufferAllocator.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocator.commandPool = g_CommandPool;
    commandBufferAllocator.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocator.commandBufferCount = static_cast<UINT>(g_vecCommandBuffers.size());
    VULKAN_ASSERT(vkAllocateCommandBuffers(renderer.GetLogicalDevice(), &commandBufferAllocator, g_vecCommandBuffers.data()), "Allocate Imgui command buffer failed");


    g_vecFrameBuffers.resize(renderer.GetSwapChainImageCount());
    for (size_t i = 0; i < renderer.GetSwapChainImageCount(); ++i)
    {
        std::vector<VkImageView> vecImageViewAttachments = {
            renderer.GetUISwapChainImageView(i),
        };

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.renderPass = g_RenderPass;
        frameBufferCreateInfo.attachmentCount = 1;
        frameBufferCreateInfo.pAttachments = vecImageViewAttachments.data();
        frameBufferCreateInfo.width = renderer.GetSwapChainExtent2D().width;
        frameBufferCreateInfo.height = renderer.GetSwapChainExtent2D().height;
        frameBufferCreateInfo.layers = 1;

        VULKAN_ASSERT(vkCreateFramebuffer(renderer.GetLogicalDevice(), &frameBufferCreateInfo, nullptr, &g_vecFrameBuffers[i]), "Create ImGui frame buffer failed");
    }

    //设置ImGui上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    //设置ImGui风格
    ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    //ImGuiStyle& style = ImGui::GetStyle();
    //if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    //{
    //    style.WindowRounding = 0.0f;
    //    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    //}

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(renderer.GetWindow(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = renderer.GetInstance();
    init_info.PhysicalDevice = renderer.GetPhysicalDevice();
    init_info.Device = renderer.GetLogicalDevice();
    init_info.QueueFamily = renderer.GetGraphicQueueIdx();
    init_info.Queue = renderer.GetGraphicQueue();
    init_info.PipelineCache = VK_NULL_HANDLE; //todo:PipelineCache是否需要
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Allocator = VK_NULL_HANDLE; //todo:Allocator是否需要
    init_info.MinImageCount = renderer.GetSwapChainMinImageCount();
    init_info.ImageCount = renderer.GetSwapChainImageCount();
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, g_RenderPass);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("./Submodule/ImGui/misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("./Submodule/ImGui/misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("./Submodule/ImGui/misc/fonts/DroidSans.ttf", 16.0f);
    //io.FontDefault = io.Fonts->AddFontFromFileTTF("./Submodule/ImGui/misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    // 
    // Upload Fonts
    //{
        // Use any command queue
        //VkCommandPool command_pool = renderer.GetTransferCommandPool();
        VkCommandBuffer command_buffer = renderer.BeginSingleTimeCommandBuffer();

        //VULKAN_ASSERT(vkResetCommandPool(renderer.GetLogicalDevice(), command_pool, 0), "Reset command pool failed");

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        renderer.EndSingleTimeCommandBuffer(command_buffer);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    //}
}

static void setStyle(uint32_t index)
{
    switch (index)
    {
    case 0:
    {
        ImGuiStyle& style = ImGui::GetStyle();

        auto vulkanStyle = ImGui::GetStyle();
        vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

        style = vulkanStyle;
        break;
    }
    case 1:
        ImGui::StyleColorsClassic();
        break;
    case 2:
        ImGui::StyleColorsDark();
        break;
    case 3:
        ImGui::StyleColorsLight();
        break;
    }
}

VkCommandBuffer& UI::FillCommandBuffer(VulkanRenderer& renderer, UINT uiIdx)
{
    static bool show_demo_window = true;
    static bool show_another_window = true;
    glm::vec3 clear_color = { 0.f, 0.f, 0.f };

    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::StyleColorsDark();


    static int selectedStyle = 0;
    ImGui::Begin("Select Mode");
    //if (ImGui::Combo("UI style", &selectedStyle, "Vulkan\0Classic\0Dark\0Light\0")) {
    //    //setStyle(selectedStyle);
    //}

    ImGui::End();

    // Rendering
    ImGui::Render();
    ImDrawData* main_draw_data = ImGui::GetDrawData();
    const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
    if (!main_is_minimized)
    {
        //vkResetCommandPool(renderer.GetLogicalDevice(), g_CommandPool, 0);

        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(g_vecCommandBuffers[uiIdx], &commandBufferBeginInfo);

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = g_RenderPass;
        renderPassBeginInfo.framebuffer = g_vecFrameBuffers[uiIdx];
        renderPassBeginInfo.renderArea.extent = renderer.GetSwapChainExtent2D();
        VkClearValue clearColor;
        clearColor.color = { 1.f, 0.f, 0.f, 1.f };
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(g_vecCommandBuffers[uiIdx], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(main_draw_data, g_vecCommandBuffers[uiIdx]);

        // Submit command buffer
        vkCmdEndRenderPass(g_vecCommandBuffers[uiIdx]);

        vkEndCommandBuffer(g_vecCommandBuffers[uiIdx]);
    }

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    return g_vecCommandBuffers[uiIdx];
}

void UI::Clean()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UI::Render_Begin()
{
    // Resize swap chain?
   //if (g_SwapChainRebuild && g_SwapChainResizeWidth > 0 && g_SwapChainResizeHeight > 0)
   //{
   //    g_SwapChainRebuild = false;
   //    ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
   //    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, g_SwapChainResizeWidth, g_SwapChainResizeHeight, g_MinImageCount);
   //    g_MainWindowData.FrameIndex = 0;
   //}

   // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::Render_End()
{

}

void UI::testSameRenderPass(VkCommandBuffer& commandBuffer)
{
    ImGui::ShowDemoWindow();


    ImGui::Render();
    ImDrawData* main_draw_data = ImGui::GetDrawData();

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);

    // Update and Render additional Platform Windows
    //ImGuiIO& io = ImGui::GetIO();
    //if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    //{
    //    ImGui::UpdatePlatformWindows();
    //    ImGui::RenderPlatformWindowsDefault();
    //}
}