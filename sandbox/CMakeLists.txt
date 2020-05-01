# configuration
set(PRECOMPILED_LIBS "$ENV{userprofile}/Binaries")

#add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

# librealsense
set(rs2_DIR ${PRECOMPILED_LIBS}/librealsense2)
link_directories(${rs2_DIR}/lib)

# opencv
list(APPEND CMAKE_MODULE_PATH ${PRECOMPILED_LIBS}/OpenCV)
find_package(OpenCV REQUIRED HINTS ${PRECOMPILED_LIBS}/OpenCV)

# Realsense Sandbox
add_executable(realsense_sandbox src/realsense_sandbox.cpp)
target_include_directories(realsense_sandbox PRIVATE ${OpenCV_INCLUDE_DIRS} ${rs2_DIR}/include include)
target_link_libraries(realsense_sandbox spdlog fmt ${OpenCV_LIBS} realsense2d)