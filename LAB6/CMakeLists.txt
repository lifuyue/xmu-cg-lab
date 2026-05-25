cmake_minimum_required(VERSION 3.10)

project(Exp6RayTracing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(exp6_raytracing ${SOURCES})
target_link_libraries(exp6_raytracing PRIVATE Eigen3::Eigen)

if(MINGW)
	target_link_options(exp6_raytracing PRIVATE -static -static-libgcc -static-libstdc++)
endif()
