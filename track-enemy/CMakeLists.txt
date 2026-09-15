cmake_minimum_required(VERSION 3.23)
project(track-enemy)

include(FetchContent)

# Download spdlog automatically during configuration
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)

FetchContent_MakeAvailable(spdlog)


# The CONFIGURE_DEPENDS keyword is a modern CMake feature that tells it: "Hey, keep an eye on this folder. If any new main.cpp files show up, automatically reconfigure the project and compile them."
# Source files
file(GLOB SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

file(GLOB HEADERS CONFIGURE_DEPENDS
    include/*.hpp
    include/*.h
)

# Create executable
add_executable(track-enemy
    ${SOURCES}
    ${HEADERS}
)

# Include project headers
target_include_directories(track-enemy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(track-enemy PRIVATE
    spdlog::spdlog
    gdiplus
    $<$<BOOL:${MINGW}>:ws2_32>
)

# C++ standard
set_target_properties(track-enemy PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
