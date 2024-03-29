newoption {
    trigger = "lstd-windows-link-runtime-library",
    description = [[ 
        Force linking with the C/C++ runtime library on Windows.
        By default, we don't and we try our hardest to provide 
        functionality to external libraries that may require it.

        If a library doesn't find a function you can just define it 
        in the static lstd library and it'll find it.

        On Linux we can't not-link with glibc, because it's 
        coupled with the POSIX operating system calls,
        although they really should be separate. 
                  ]]
}

function setup_configurations()
    filter "configurations:Debug"
        defines "DEBUG"
		
		-- Trips an assert if you try to access an element out of bounds.
		-- Works for arrays and strings in the library. I don't think we can check raw C arrays...
		defines { "LSTD_ARRAY_BOUNDS_CHECK", "LSTD_NUMERIC_CAST_CHECK" }
		
        symbols "On"

    filter "configurations:DebugOptimized"
        defines { "DEBUG", "DEBUG_OPTIMIZED" }
        
		defines { "LSTD_ARRAY_BOUNDS_CHECK", "LSTD_NUMERIC_CAST_CHECK" }
		
		optimize "On"
        symbols "On"

    filter { "system:windows", "configurations:DebugOptimized", "options:not lstd-windows-link-runtime-library" }
		-- Otherwise MSVC generates internal undocumented intrinsics which we can't provide .. shame
		floatingpoint "Strict"
    
    filter "configurations:Release"
        defines { "RELEASE", "NDEBUG" } 

        optimize "Full"
		symbols "Off"

    filter { "system:windows", "configurations:Release", "options:not lstd-windows-link-runtime-library" }
		-- Otherwise MSVC generates internal undocumented intrinsics which we can't provide .. shame
		floatingpoint "Strict"
		
	filter {}
end

function link_lstd()
    filter {"kind:not StaticLib"} 
        links { "lstd" }
    filter {}

    if LSTD_NAMESPACE then
        print("Building library with namespace: \"" .. LSTD_NAMESPACE .. "\"")

        if LSTD_NAMESPACE ~= "" then
            defines { "LSTD_NAMESPACE=" .. LSTD_NAMESPACE }
        else
            defines { "LSTD_NO_NAMESPACE" }
        end
    else
        print("Building library with no namespace")
        defines { "LSTD_NO_NAMESPACE" }
    end

	filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }

        -- We need _CRT_SUPPRESS_RESTRICT for some reason
        defines { "NOMINMAX", "WIN32_LEAN_AND_MEAN", "_CRT_SUPPRESS_RESTRICT" }
        
        links { "dbghelp" }

        -- FreeType
	    includedirs { "vendor/Windows/freetype/include" }
	    links { "vendor/Windows/freetype/freetype.lib" }
    
    filter { "system:windows", "options:not lstd-windows-link-runtime-library" }
        rtti "Off"
        justmycode "Off"
        editandcontinue "Off"
        exceptionhandling "Off" -- SEH still work, which are required for some CRT stuff 

        -- Even if we don't link with the runtime library, the following 
        -- line makes certain warnings having to do with us replacing malloc/free disappear. 
	    -- (Certain CRT headers are included which define those functions with 
	    -- __declspec(dllimport) which don't match with our definitions).
	    --
	    -- As for why CRT headers are somehow always included? Ask Windows.h .....
	    staticruntime "On"

        defines { "LSTD_NO_CRT" }
		flags { "OmitDefaultLibrary", "NoRuntimeChecks", "NoBufferSecurityCheck", "NoIncrementalLink" }
		buildoptions { "/Gs9999999" }   

    filter { "system:windows", "kind:ConsoleApp or SharedLib", "options:not lstd-windows-link-runtime-library" }
		linkoptions { "/nodefaultlib", "/subsystem:windows", "/stack:\"0x100000\",\"0x100000\"" }
        links { "kernel32", "shell32" }
        
    -- Setup entry point
    filter { "system:windows", "kind:SharedLib", "options:not lstd-windows-link-runtime-library" }
	    entrypoint "main_no_crt_dll"
    filter { "system:windows", "kind:ConsoleApp or WindowedApp", "options:not lstd-windows-link-runtime-library" }
	    entrypoint "main_no_crt"
    filter {}

    setup_configurations()
end

function add_files(str)
	files {
        "include/" .. str .. "/**.h",
        "include/" .. str .. "/**.inc",
        "include/" .. str .. "/**.def",
        "src/" .. str .. "/**.c",
        "src/" .. str .. "/**.cpp"
    }
end

function add_files_for_extra(extra)
    add_files("lstd_extra/" .. extra)
end

project "lstd"
    kind "StaticLib"
    architecture "x64"

    language "C++" 
    cppdialect "C++20"     
	
	characterset "Unicode"
    
    targetdir(OUT_DIR)
    objdir(INT_DIR)

    if LSTD_PLATFORM_TEMPORARY_STORAGE_STARTING_SIZE then
        defines { "PLATFORM_TEMPORARY_STORAGE_STARTING_SIZE=" .. LSTD_PLATFORM_TEMPORARY_STORAGE_STARTING_SIZE }
    else 
        defines { "PLATFORM_TEMPORARY_STORAGE_STARTING_SIZE=16_KiB" }
    end

    if LSTD_PLATFORM_PERSISTENT_STORAGE_STARTING_SIZE then
        defines { "PLATFORM_PERSISTENT_STORAGE_STARTING_SIZE=" .. LSTD_PLATFORM_PERSISTENT_STORAGE_STARTING_SIZE }
    else 
        defines { "PLATFORM_PERSISTENT_STORAGE_STARTING_SIZE=1_MiB" }
    end
    
    includedirs { "include/", "include/lstd/vendor/cephes/cmath/" }
    add_files("lstd")
    add_files("lstd-graphics")

	files {
		"src/lstd/lstd.natvis",
		"src/lstd-graphics/imgui.natvis"
	}
	includedirs { "%{prj.name}/src/vendor/imgui" }	
	
	excludes { "lstd/src/vendor/imguizmo/**" }

    filter { "system:linux" }
        removefiles { "src/lstd/platform/windows/**" }
        removefiles { "src/lstd/vendor/cephes/**" }

    filter { "system:macosx" }
        removefiles { "src/lstd/platform/windows/no_crt/**" }
        removefiles { "src/lstd/platform/windows/**" }
        removefiles { "src/lstd/vendor/cephes/**" }
    
    filter { "system:windows"}
        removefiles { "src/lstd/platform/posix/**" }

    filter { "system:windows", "options:not lstd-windows-link-runtime-library"}
        -- These are x86-64 assembly and obj files since we don't support 
        -- other architectures at the moment.
        files {
            "src/lstd/platform/windows/no_crt/longjmp_setjmp.asm",
            "src/lstd/platform/windows/no_crt/chkstk.asm"
        }

    filter { "system:windows", "options:lstd-windows-link-runtime-library"}
        removefiles { "src/lstd/platform/windows/no_crt/**" }
        removefiles { "src/lstd/vendor/cephes/**" }
    filter {}

    if LSTD_INCLUDE_EXTRAS then
        for _, extra in ipairs(LSTD_INCLUDE_EXTRAS) do
            add_files_for_extra(extra)
        end
    end

    link_lstd() -- This is just to get the common options, not actually linking.
	


