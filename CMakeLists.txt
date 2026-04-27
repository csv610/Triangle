cmake_minimum_required(VERSION 3.14)
project(TriangleMesh VERSION 1.6.0 LANGUAGES C CXX)

# --- Options ---
option(BUILD_GUI "Build the interactive ImGui tool" ON)
option(BUILD_BENCHMARKS "Build benchmark executables" ON)
option(BUILD_TESTS "Build unit tests" ON)

# --- Global Configuration ---
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

# Standard Triangle definitions
set(TRIANGLE_DEFS 
    REAL=double 
    ANSI_DECLARATORS 
    NO_TIMER
)

# --- Dependencies ---
find_package(Threads REQUIRED)

include(FetchContent)

if(BUILD_GUI)
    find_package(glfw3 3.3 QUIET)
    if(NOT glfw3_FOUND)
        # Fallback for systems where glfw3 is named glfw
        find_package(glfw 3.3 REQUIRED)
        set(glfw3_LIBRARIES glfw)
    else()
        set(glfw3_LIBRARIES glfw)
    endif()
    
    find_package(OpenGL REQUIRED)
    
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
    )
    FetchContent_MakeAvailable(imgui)
endif()

if(BUILD_TESTS)
    find_package(Catch2 3 REQUIRED)
endif()

# --- Core Library ---
# ... (rest of core library)
# This library is used by the GUI, benchmarks, and other C++ applications.
add_library(mesh_core STATIC
    src/triangle.c
    src/triangle.h
    cpp/TriMeshGenerator.cpp
    cpp/TriMeshGenerator.hpp
    cpp/MeshOptimizer.cpp
    cpp/MeshOptimizer.hpp
)

target_compile_definitions(mesh_core PRIVATE ${TRIANGLE_DEFS} TRILIBRARY)
target_include_directories(mesh_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp>
)
target_link_libraries(mesh_core PRIVATE Threads::Threads)
if(UNIX AND NOT APPLE)
    target_link_libraries(mesh_core PUBLIC m)
endif()

# --- Standalone Triangle Executable ---
add_executable(triangle src/triangle.c)
target_compile_definitions(triangle PRIVATE ${TRIANGLE_DEFS})
if(UNIX AND NOT APPLE)
    target_link_libraries(triangle PRIVATE m)
endif()

# --- Interactive GUI ---
if(BUILD_GUI)
    set(IMGUI_SRCS
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
    )

    add_executable(interactive_mesh vis/main_gui.cpp ${IMGUI_SRCS})
    
    target_include_directories(interactive_mesh PRIVATE 
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    
    target_link_libraries(interactive_mesh PRIVATE 
        mesh_core
        ${glfw3_LIBRARIES}
        OpenGL::GL
        Threads::Threads
    )

    if(APPLE)
        target_link_libraries(interactive_mesh PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
    endif()
endif()

# --- Benchmarks ---
if(BUILD_BENCHMARKS)
    foreach(bench benchmark_hull benchmark_delaunay benchmark_voronoi)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/${bench}.c")
            add_executable(${bench} bench/${bench}.c)
            target_link_libraries(${bench} PRIVATE mesh_core)
        endif()
    endforeach()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/benchmark_cpp_wrapper.cpp")
        add_executable(benchmark_cpp_wrapper bench/benchmark_cpp_wrapper.cpp)
        target_link_libraries(benchmark_cpp_wrapper PRIVATE mesh_core)
    endif()
endif()

# --- Unit Tests ---
if(BUILD_TESTS)
    enable_testing()
    
    # Catch2 Tests
    add_executable(unit_tests tests/test_mesh.cpp)
    target_link_libraries(unit_tests PRIVATE mesh_core Catch2::Catch2WithMain)
    
    include(Catch)
    catch_discover_tests(unit_tests)
endif()

# --- Installation (Optional) ---
install(TARGETS triangle mesh_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
