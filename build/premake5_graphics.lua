function common_settings_graphics()
	links { "lstd" }

	-- FreeType
	includedirs { BASE_DIR .. "third_party/freetype2/include" }
	links { BASE_DIR .. "third_party/freetype2/freetype.lib" }

    common_settings()
end

project "lstd-graphics"
    kind "StaticLib"
    
	files {
		BASE_DIR .. "src/lstd-graphics/imgui.natvis"
	}
	
	common_settings_graphics()


-- Which DLL should the driver run, should be the name of one of the projects here in premake
VEHICLE = "graph"

project "driver"
    kind "ConsoleApp"
	
	defines { "BUILDING_DRIVER", "VEHICLE=" .. VEHICLE }
	
    links { "lstd-graphics" }
	
	dependson { VEHICLE }
	debugdir(BASE_DIR .. "../src/" .. VEHICLE .. "/")
	
    common_settings_graphics()
	
	filter "system:windows"
		links { "imm32", "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }

project "graph"
	-- We build any program logic as a DLL in order to be able to 
	-- hot-reload it after doing changes, without losing state.
    kind "SharedLib" 

    links { "lstd-graphics" }
	includedirs(BASE_DIR .. "src/driver/")
    
    common_settings_graphics()

	-- Output dll in the same location as the driver
    targetdir( BASE_DIR .. "bin/%{cfg.buildcfg}/driver" )
	
    filter "system:windows"
		-- Unique PDB name each time we build (in order to support debugging while hot-swapping the game dll)
		-- This stopped working in VS2022, will try again.
        -- symbolspath '$(OutDir)$(TargetName)-$([System.DateTime]::Now.ToString("ddMMyyyy_HHmmss_fff")).pdb'
        links { "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }
