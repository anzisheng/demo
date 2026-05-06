# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\WaterCurtain_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\WaterCurtain_autogen.dir\\ParseCache.txt"
  "WaterCurtain_autogen"
  )
endif()
