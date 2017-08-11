solution "a0"
  configurations { "Debug", "Release" }
  
  configuration { "Debug" }
    targetdir "debug"
    buildoptions{"-std=c++0x"}
 
  configuration { "Release" }
    targetdir "release"
    buildoptions{"-std=c++0x"}
  include("vecmath")
project "a0"
  language "C++"
  kind     "ConsoleApp"
  files  { "src/**.cpp" }
  includedirs {"./include/vecmath"}
  --For windows
  --this directory should be changed to windows specific directory
  links {"GL","GLU","glut","vecmath"}
  
  configuration { "Debug*" }
    defines { "_DEBUG", "DEBUG" }
    flags   { "Symbols" }

  configuration { "Release*" }
    defines { "NDEBUG" }
    flags   { "Optimize" } 
