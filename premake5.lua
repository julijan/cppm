workspace "cppm"
	configurations { "Debug", "Release" }

project "cppm"
	kind "ConsoleApp"
	language "C++"
	architecture "x64"
	targetdir "bin/%{cfg.buildcfg}"
	includedirs { "/home/julijan/.vcpkg/installed/x64-linux/include" }
	libdirs { "/home/julijan/.vcpkg/installed/x64-linux/lib" }
	links { "boost_json" }
	files { "./src/**.h", "./src/**.cpp" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"