project "lstd-graphics"
    kind "StaticLib"
    
	links { "lstd" }

	files {
		BASE_DIR .. "src/lstd-graphics/imgui.natvis"
	}
	
    common_settings()

    excludes { BASE_DIR .. "src/lstd-graphics/third_party/imguizmo/**" }


-- Which DLL should the driver run, should be the name of one of the projects here in premake
VEHICLE = "graph"

project "driver"
    kind "ConsoleApp"
	
	defines { "BUILDING_DRIVER", "VEHICLE=" .. VEHICLE }
	
    links { "lstd", "lstd-graphics" }
	
	dependson { VEHICLE }
	debugdir(VEHICLE .. "/")
	
    common_settings()
	
	filter "system:windows"
		links { "imm32", "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }

project "graph"
	-- We build any program logic as a DLL in order to be able to 
	-- hot-reload it after doing changes, without losing state.
    kind "SharedLib" 

    links { "lstd", "lstd-graphics" }
    
    common_settings()

	-- Output dll in the same location as the driver
    targetdir( BASE_DIR .. "bin/%{cfg.buildcfg}/driver" )
	
    filter "system:windows"
		-- Unique PDB name each time we build (in order to support debugging while hot-swapping the game dll)
		-- This stopped working in VS2022, will try again.
        -- symbolspath '$(OutDir)$(TargetName)-$([System.DateTime]::Now.ToString("ddMMyyyy_HHmmss_fff")).pdb'
        links { "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }
