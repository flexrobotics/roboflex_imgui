cmake_minimum_required(VERSION 3.18)

project(roboflex_imgui)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# -------------------- 
# Resolve dependencies

include(FetchContent)

find_package(SDL2 REQUIRED)
find_package(GLEW 2.0 REQUIRED)

include_directories(${GLFW_INCLUDE_DIRS})

message("GLEW_INCLUDE_DIRS: ${GLEW_INCLUDE_DIRS}")
message("GLEW_LIBRARIES: ${GLEW_LIBRARIES}")

# Locate the installed roboflex_core and its dependencies
# download and build roboflex_core
FetchContent_Declare(roboflex_core
    GIT_REPOSITORY https://github.com/flexrobotics/roboflex.git
    GIT_TAG        main
    DOWNLOAD_EXTRACT_TIMESTAMP 1
)
set(BUILD_ROBOFLEX_PYTHON_EXT OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(roboflex_core)

# ONLY IF YOU ARE COMPILING THE EXAMPLE
# TODO: move into examples/CMakeLists.txt
# download and build roboflex_transport_zmq
FetchContent_Declare(roboflex_transport_zmq
    GIT_REPOSITORY https://github.com/flexrobotics/roboflex_transport_zmq.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(roboflex_transport_zmq)

# Fetch and make imgui available, and build static library ourselves
FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.89.5.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP 1
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
  DOWNLOAD_EXTRACT_TIMESTAMP 1
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
    src/implot_node.cpp
    src/oned_television.cpp
    src/metrics_television.cpp
    include/roboflex_imgui/implot_node.h
    include/roboflex_imgui/oned_television.h
    include/roboflex_imgui/metrics_television.h
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
    imgui
    implot
    ${GLEW_LIBRARIES}
    ${SDL2_LIBRARIES}
)
if (APPLE)
    message("IS APPLE!!!")
    target_link_libraries(roboflex_imgui PUBLIC ${GLFW_STATIC_LIBRARIES})
    target_link_libraries(roboflex_imgui PUBLIC "-framework OpenGL")
else()
    target_link_libraries(roboflex_imgui PUBLIC GL)
endif()


# -------------------- 
# Examples

# display_oneD_data_cpp example
add_executable(display_oneD_data_cpp examples/display_oneD_data_cpp.cpp)
target_link_libraries(display_oneD_data_cpp PUBLIC 
    roboflex_core 
    roboflex_imgui
    roboflex_transport_zmq
)

# show_blank_cpp example
add_executable(show_blank_cpp examples/show_blank_cpp.cpp)
target_link_libraries(show_blank_cpp PUBLIC 
    roboflex_core 
    roboflex_imgui
)


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
