workspace "VulkanRenderer"
    configurations { "Debug", "Release" }

project "VulkanRenderer"
    kind "ConsoleApp"
    language "C++"

    targetdir "bin/%{cfg.buildcfg}"

    architecture "x64"

    files
    {
        "./Source/*.h", 
        "./Source/*.cpp",
    }

    libdirs { "D:/VulkanSDK/Lib" } --附加库目录

    links --附加依赖项
    {
        "vulkan-1.lib",
        "glfw3.lib"
    }

    includedirs --外部包含目录
    {
        "./Submodule/GLFW/include",
        "./Submodule/glm",
        "./Submodule/spdlog/include",
        "D:/VulkanSDK/Include",
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"