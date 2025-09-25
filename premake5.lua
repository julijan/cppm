workspace "cppm"
	configurations { "Debug", "Release" }

project "cppm"
	language "C++"
	kind "ConsoleApp"
	cppdialect "C++20"
	architecture "x64"
	targetdir "bin/%{cfg.buildcfg}"
	files { "./src/**.h", "./src/**.cpp" }
	includedirs { "./includes/src/**", "./includes/uses/src/**" }
	libdirs { "./includes/lib/**", "./includes/uses/lib/**" }
	links {
		"print-nice",
		"text-styled",
		"boost_json",
		"boost_json",
		"boost_container",
		"boost_container",
		"boost_system",
		"boost_system",
		"fmt",
		"fmtd"
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
