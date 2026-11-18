cmake_minimum_required(VERSION 3.28.2)

# Get version
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/props.json PROPS)
string(JSON VER GET ${PROPS} version)

project(cmdrunner
  DESCRIPTION "Run commands."
  VERSION ${VER})

set(CMAKE_CXX_STANDARD 26)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Add the path to the source files for cmdrunner
file(GLOB_RECURSE CMDRUNNER_SOURCE_FILES "src/*.cpp")

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Add the executable target for cmdrunner
add_executable(cmdrunner ${CMDRUNNER_SOURCE_FILES})
install(TARGETS cmdrunner DESTINATION bin)
