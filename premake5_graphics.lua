
-- Which dll should the driver run
VEHICLE = "graph"

project "driver"
	location "%{prj.name}"
    kind "ConsoleApp"
	
	defines { "BUILDING_DRIVER", "VEHICLE=" .. VEHICLE }
	
	includedirs { "lstd/src" }     
    links { "lstd" }
	
	dependson { VEHICLE }
	debugdir(VEHICLE .. "/")
	
    common_settings()
	
	filter "system:windows"
		links { "imm32", "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }

project "graph"
    location "%{prj.name}"
    kind "SharedLib"

	includedirs { "lstd/src", "driver/src" } 
    links { "lstd" }
    
    common_settings()

	-- Output dll in the same location as the driver
    targetdir("bin/%{cfg.buildcfg}/driver")
	
	filter "action:vs*"
		pchheader "pch.h"
		pchsource "%{prj.name}/src/pch.cpp"
	filter {} -- @Platform Other compilers...
	
    -- Unique PDB name each time we build (in order to support debugging while hot-swapping the game dll)
    filter "system:windows"
        symbolspath '$(OutDir)$(TargetName)-$([System.DateTime]::Now.ToString("ddMMyyyy_HHmmss_fff")).pdb'
        links { "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }

project "engine"
    location "%{prj.name}"
    kind "SharedLib"

	includedirs { "lstd/src", "driver/src" } 
    links { "lstd" }
    
    common_settings()
	
	-- Output dll in the same location as the driver
    targetdir("bin/%{cfg.buildcfg}/driver")
	
    -- Unique PDB name each time we build (in order to support debugging while hot-swapping the game dll)
    filter "system:windows"
        symbolspath '$(OutDir)$(TargetName)-$([System.DateTime]::Now.ToString("ddMMyyyy_HHmmss_fff")).pdb'
        links { "dxgi.lib", "d3d11.lib", "d3dcompiler.lib", "d3d11.lib", "d3d10.lib" }
	