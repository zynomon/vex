# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\vex_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\vex_autogen.dir\\ParseCache.txt"
  "vex_autogen"
  )
endif()
