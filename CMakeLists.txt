cmake_minimum_required(VERSION 3.17)
project(raylib_sandbox)

set(CMAKE_CXX_STANDARD 11)

# if raylib isn't already available on this system, fetch and build it
find_package(raylib 3.5.0 QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            raylib
            URL https://github.com/raysan5/raylib/archive/master.tar.gz
    )

    FetchContent_GetProperties(raylib)
    if (NOT raylib_POPULATED) # have we downloaded raylib yet?
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(raylib)

        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build supplied examples
        set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build supplied games

        # build raylib
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif()
endif()

add_executable(${PROJECT_NAME}
        src/main.c
        src/assets.c
        src/assets.h
        src/world.c
        src/world.h
        lib/stb/stb.h
        )
target_link_libraries(${PROJECT_NAME} raylib)
