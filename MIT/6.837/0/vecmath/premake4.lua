project "vecmath"
  kind "StaticLib"
  language "C++"
  files{
    "**.cpp"
  }
  includedirs{"../include/vecmath"}
  configuration {"Debug"}
  defines{"Debug"}
  flags{"Symbols"}
  
  configuration {"Release"}
  defines{"NDebug"}
  flags{"Optimize"}
  
