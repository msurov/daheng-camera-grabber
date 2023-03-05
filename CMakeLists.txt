cmake_minimum_required(VERSION 3.2)
project(dahengdemo CXX)

find_package(GXI REQUIRED PATHS "3rd_party/gxi")
find_package(OpenCV REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(dahengdemo
    src/main.cpp
    src/gxcam.h
    src/gxcam.cpp
)
target_include_directories(dahengdemo PUBLIC ${GXI_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(dahengdemo ${GXI_LIBRARIES} ${OpenCV_LIBRARIES} pthread)
