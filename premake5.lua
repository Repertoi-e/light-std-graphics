
workspace "light-std"
    architecture "x64"
    configurations { "Debug", "DebugOptimized", "Release" }

function common_settings()
    architecture "x64"

    language "C++"

    -- We can't specify C++20 but at least on Windows, our generate_projects.bat replaces language standard with stdcpplatest in the .vcxproj files
    cppdialect "C++17" 

    rtti "Off"
    characterset "Unicode"
    
    editandcontinue "Off"
    exceptionhandling "Off" -- SEH still works
    
    -- Define this if you include headers from the normal standard library (STL).
    -- If this macro is not defined we provide our own implementations of certain things 
    -- that are normally defined in the STL and on which certain C++ features rely on.
    -- (e.g. the compare header - required by the spaceship operator, placement new and initializer_lists)
    -- defines { "LSTD_DONT_DEFINE_STD" }
    
    includedirs { "%{prj.name}/src" }
    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }
        
        excludes "%{prj.name}/**/posix_*.cpp"

        -- We need _CRT_SUPPRESS_RESTRICT for some dumb reason
        defines { "LSTD_NO_CRT", "NOMINMAX", "WIN32_LEAN_AND_MEAN", "_CRT_SUPPRESS_RESTRICT" } 
    
        buildoptions { "/Gs9999999" }
        
        links { "dwmapi.lib", "dbghelp.lib" }
        flags { "OmitDefaultLibrary", "NoRuntimeChecks", "NoBufferSecurityCheck" }
    filter { "system:windows", "not kind:StaticLib" }
        linkoptions { "/nodefaultlib", "/subsystem:windows", "/stack:\"0x100000\",\"0x100000\"" }
        links { "kernel32", "shell32", "winmm", "ole32" }
        
    -- Setup entry point
    filter { "system:windows", "kind:SharedLib" }
        entrypoint "main_no_crt_dll"
    filter { "system:windows", "kind:ConsoleApp or WindowedApp" }
        entrypoint "main_no_crt"

    -- Setup configurations and optimization level
    filter "configurations:Debug"
        defines "DEBUG"
        symbols "On"
        buildoptions { "/FS" }
    filter "configurations:DebugOptimized"
        defines { "DEBUG", "DEBUG_OPTIMIZED" }
        optimize "On"
        symbols "On"
        buildoptions { "/FS" }
    filter "configurations:Release"
        defines { "RELEASE", "NDEBUG" } 
        optimize "Full"
        floatingpoint "Fast"
    filter {}
end


outputFolder = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


project "lstd"
    location "%{prj.name}"
    kind "StaticLib"

    targetdir("bin/" .. outputFolder .. "/%{prj.name}")
    objdir("bin-int/" .. outputFolder .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.inc",
        "%{prj.name}/src/**.c",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.def",
        "%{prj.name}/src/**.ixx",
        "%{prj.name}/src/**.obj"
    }
    
	includedirs { "%{prj.name}/src/vendor/linasm/include" }
    
    common_settings()


project "lstd_graphics"
    location "%{prj.name}"
    kind "StaticLib"

    targetdir("bin/" .. outputFolder .. "/%{prj.name}")
    objdir("bin-int/" .. outputFolder .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.inc",
        "%{prj.name}/src/**.c",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.def",
        "%{prj.name}/src/**.ixx"
    }

    includedirs { "lstd/src", "lstd/src/vendor/linasm/include" } -- @TODO
    
    links { "lstd" } -- This adds a reference in VS, doesn't actually link (they are static libs).
	
    pchheader "pch.h"
    pchsource "%{prj.name}/src/pch.cpp"
    forceincludes { "pch.h" }
    
    common_settings()

project "graph"
	location "%{prj.name}"
    kind "ConsoleApp"
    
	targetdir("bin/" .. outputFolder .. "/%{prj.name}")
    objdir("bin-int/" .. outputFolder .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.inc",
        "%{prj.name}/src/**.c",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.def",
        "%{prj.name}/src/**.ixx"
    }
	
	-- Exclude directx files on non-windows platforms 
    filter "not system:windows"
        excludes  { "%{prj.name}/src/d3d_*.h", "%{prj.name}/src/d3d_*.cpp" }
	filter {}
	
	includedirs { "lstd/src", "lstd_graphics/src", "lstd/src/vendor/linasm/include" } -- @TODO
    
    links { "lstd", "lstd_graphics" }
    --pchheader "pch.h"
    --pchsource "%{prj.name}/src/pch.cpp"
    --forceincludes { "pch.h" }
    
    common_settings()
	