cmake_minimum_required(VERSION 4.2)
project(raylib_sandbox C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -- raylib: shared, so platform.exe and game.dll share one set of globals --
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND ON  CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11     OFF CACHE BOOL "" FORCE)
set(FLECS_STATIC       OFF CACHE BOOL "" FORCE)
set(FLECS_SHARED       ON  CACHE BOOL "" FORCE)

FetchContent_Declare(raylib
        GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
        GIT_TAG        "6.0"
        GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(flecs
        GIT_REPOSITORY "https://github.com/SanderMertens/flecs.git"
        GIT_TAG        "v4.1.5"
        GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(flecs)

# All artifacts in one place so the loaded game.dll and raylib.dll sit next
# to platform.exe at runtime. Set the CLion run config's working directory
# to this dir, or use GetApplicationDirectory() in platform.c.
set(RUNTIME_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${RUNTIME_DIR}")

set(SOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/sources")

# -- Shared module: compiled into both game and platform --
# NOTE: ***DO NOT*** use file-static vars in ${SOURCES_DIR}/shared/ —
# each consumer gets its own copy. Communicate through GameMemory instead.
file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS "${SOURCES_DIR}/shared/*.c")
add_library(shared OBJECT ${SHARED_SOURCES})
set_target_properties(shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(shared PUBLIC "${SOURCES_DIR}")
target_link_libraries(shared PUBLIC raylib)
target_link_libraries(shared PUBLIC flecs)

# -- Game module: shared library, hot-swappable --
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${SOURCES_DIR}/game/*.c")
add_library(game SHARED ${GAME_SOURCES})
set_target_properties(game PROPERTIES
        C_VISIBILITY_PRESET hidden          # only GAME_EXPORT symbols escape
        POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(game PRIVATE shared)

# Default suffixes give us:  Linux libgame.so | Windows game.dll | macOS libgame.dylib
# platform.c already expects libgame.so / game.dll, so no PREFIX override.

# -- Platform executable --
file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS "${SOURCES_DIR}/platform/*.c")
add_executable(${PROJECT_NAME} ${PLATFORM_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE shared)

# Stage raylib.dll, flecs.dll next to platform.exe / game.dll on every build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
)

# Building the exe target also builds the game module — and the "game"
# target on its own is what you'll trigger from CLion to do a hot rebuild.
add_dependencies(${PROJECT_NAME} game)

# -- MSVC: minimize PDB lock pain --
if(MSVC)
    # Fresh PDB name per CMake configure. Touch CMakeCache.txt (or run
    # "Reset Cache and Reload Project" in CLion) when you start a debug
    # session to get a clean PDB. For pure no-debugger hot-reload runs,
    # the copy-before-load trick in platform.c is sufficient on its own.
    string(TIMESTAMP BUILD_STAMP "%Y%m%d%H%M%S")
    set_target_properties(game PROPERTIES PDB_NAME "game_${BUILD_STAMP}")
endif()
