
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
        
		
		-- @TODO We are linking with the CRT for now... 
        -- flags { "OmitDefaultLibrary", "NoRuntimeChecks", "NoBufferSecurityCheck" }
    filter { "system:windows", "not kind:StaticLib" }
		-- @TODO We are linking with the CRT for now... 
        -- linkoptions { "/nodefaultlib", "/subsystem:windows", "/stack:\"0x100000\",\"0x100000\"" }
        links { "kernel32", "shell32", "winmm", "ole32", "dwmapi", "dbghelp" }
        
    -- Setup entry point
    -- filter { "system:windows", "kind:SharedLib" }
    --     entrypoint "main_no_crt_dll"
    -- filter { "system:windows", "kind:ConsoleApp or WindowedApp" }
    --     entrypoint "main_no_crt"

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
        "%{prj.name}/src/**.ixx"
    }
	
	excludes {
		"%{prj.name}/src/vendor/imguizmo/*.cpp",
		"%{prj.name}/src/lstd_platform/windows_no_crt/**.cpp" -- @TODO
	}
    
	filter "not system:windows"
		-- Exclude directx files on non-windows platforms 
        excludes  { "%{prj.name}/src/lstd_graphics_platform/d3d_*.cpp" }
	filter {}
    
    common_settings()

project "game"
	location "%{prj.name}"
    kind "ConsoleApp"
    
	targetdir("bin/" .. outputFolder .. "/%{prj.name}")
    objdir("bin-int/" .. outputFolder .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }
	
	excludes { 
        "%{prj.name}/src/graph/**.h", 
        "%{prj.name}/src/graph/**.cpp"
    }
	
	includedirs { "lstd/src" } 
    
    links { "lstd" }
	
	dependson { "graph" }
	
    pchheader "game.h"
    pchsource "%{prj.name}/src/game.cpp"
    forceincludes { "game.h" }
    
    common_settings()
	
	filter "system:windows"
		links { "imm32", "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }


project "graph"
    location "game"
    kind "SharedLib"

    targetdir("bin/" .. outputFolder .. "/game")
    objdir("bin-int/" .. outputFolder .. "/game/%{prj.name}")

    files {
        "game/src/graph/**.h", 
        "game/src/graph/**.cpp"
    }

    defines { "LE_BUILDING_GAME" }

    links { "lstd" }
    includedirs { "lstd/src", "game/src", "game/src/graph" } 

    pchheader "pch.h"
    pchsource "game/src/graph/pch.cpp"
    forceincludes { "pch.h" }
    
    common_settings()

    -- Unique PDB name each time we build (in order to support debugging while hot-swapping the game dll)
    filter "system:windows"
        symbolspath '$(OutDir)$(TargetName)-$([System.DateTime]::Now.ToString("ddMMyyyy_HHmmss_fff")).pdb'
        links { "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }
	