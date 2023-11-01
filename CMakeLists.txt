cmake_minimum_required(VERSION 3.24)

project(roboflex_imgui VERSION 0.1.7 DESCRIPTION "roboflex visualization/gui tools using IMGUI/IMPLOT")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# -------------------- 
# Resolve dependencies

include(FetchContent)

find_package(SDL2 REQUIRED)
find_package(GLEW 2.0 REQUIRED)

message("GLEW_INCLUDE_DIRS: ${GLEW_INCLUDE_DIRS}")
message("GLEW_LIBRARIES: ${GLEW_LIBRARIES}")

# Locate the installed roboflex_core and its dependencies
# download and build roboflex_core
FetchContent_Declare(roboflex_core
    GIT_REPOSITORY https://github.com/flexrobotics/roboflex.git
    GIT_TAG        main
)
set(BUILD_ROBOFLEX_PYTHON_EXT OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(roboflex_core)

# ONLY IF YOU ARE COMPILING THE EXAMPLE
# TODO: move into examples/CMakeLists.txt
# # download and build roboflex_transport_zmq
# FetchContent_Declare(roboflex_transport_zmq
#     GIT_REPOSITORY https://github.com/flexrobotics/roboflex_transport_zmq.git
#     GIT_TAG        main
# )
# FetchContent_MakeAvailable(roboflex_transport_zmq)

# Fetch and make imgui available, and build static library ourselves
FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.89.5.tar.gz
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_SRC_DIR "${imgui_SOURCE_DIR}")
add_library(imgui STATIC
  ${IMGUI_SRC_DIR}/imgui.cpp
  ${IMGUI_SRC_DIR}/imgui_draw.cpp
  ${IMGUI_SRC_DIR}/imgui_tables.cpp
  ${IMGUI_SRC_DIR}/imgui_widgets.cpp
  ${IMGUI_SRC_DIR}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_SRC_DIR}/backends/imgui_impl_opengl3.cpp
)
set_property(TARGET imgui PROPERTY 
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(imgui PUBLIC 
    ${IMGUI_SRC_DIR}
    ${SDL2_INCLUDE_DIRS}
)

# get and build implot the same way
FetchContent_Declare(
  implot
  URL https://github.com/epezent/implot/archive/refs/tags/v0.14.tar.gz
)
FetchContent_MakeAvailable(implot)
set(IMPLOT_SRC_DIR "${implot_SOURCE_DIR}")
add_library(implot STATIC
  ${IMPLOT_SRC_DIR}/implot.cpp
  ${IMPLOT_SRC_DIR}/implot_items.cpp
)
set_property(TARGET implot PROPERTY 
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(implot PUBLIC 
    ${IMPLOT_SRC_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${IMGUI_SRC_DIR}
)
target_compile_definitions(implot PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
)


# -------------------- 
# Define the library

add_library(roboflex_imgui STATIC
    src/imgui_nodes.cpp
    include/roboflex_imgui/imgui_nodes.h
)

# Set some properties on our library
set_property(TARGET roboflex_imgui PROPERTY 
    POSITION_INDEPENDENT_CODE ON
)

# Include directories when we compile our library
target_include_directories(roboflex_imgui PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:include>
    ${GLEW_INCLUDE_DIRS} # do we need?
    ${SDL2_INCLUDE_DIRS} # do we really need?
)

# Link against the necessary libraries
target_link_libraries(roboflex_imgui PUBLIC 
    roboflex_core 
    #roboflex_transport_zmq
    imgui
    implot
    #GLEW
    #GL
    #SDL2
    ${GLEW_LIBRARIES}
    ${SDL2_LIBRARIES}

    # Unfortunately, we need to link against all these libraries
    # because they are used by roboflex_core, and cmake doesn't
    # seem to be transitivily doing it's thing. Urgh, someone help.
    flatbuffers_util 
    xtensor 
    xsimd 
    xtl
    Eigen3::Eigen
)


# -------------------- 
# Examples

# basic_0 example
# add_executable(display_oneD_data_cpp examples/display_oneD_data_cpp.cpp)
# target_link_libraries(display_oneD_data_cpp PUBLIC 
#     roboflex_core 
#     roboflex_imgui
#     imgui
#     implot
#     GLEW
#     GL
#     SDL2
# )


# -------------------- 
# install

# If you need to install the imgui library
install(TARGETS roboflex_imgui 
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/roboflex_imgui
    DESTINATION include
)


# --------------------
# build python bindings

add_subdirectory(python)
