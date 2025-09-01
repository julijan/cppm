#pragma once

namespace Templates {
	const char* const PREMAKE = R"(workspace "${projectName}"
        configurations { "Debug", "Release" }

project "${projectName}"
        kind "${projectType}"
        language "C++"
        cppdialect "C++23"
        architecture "x64"
        targetdir "bin/%{cfg.buildcfg}"
        files { "**.h", "**.cpp" }
        links {  }
        includedirs { "./includes" }

        filter "configurations:Debug"
                defines { "DEBUG" }
                symbols "On"

        filter "configurations:Release"
                defines { "NDEBUG" }
                optimize "On"

project "${projectName}Test"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++23"
        architecture "x64"
        targetdir "tests/%{cfg.buildcfg}"
        files { "./src/test.cpp" }
        includedirs {  }
        libdirs {  }
        links { "${projectName}" }

        filter "configurations:Debug"
                defines { "DEBUG" }
                symbols "On"

        filter "configurations:Release"
                defines { "NDEBUG" }
                optimize "On")";
}