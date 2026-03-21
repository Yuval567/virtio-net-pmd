cmake_minimum_required(VERSION 3.10)

project(Userspace-Virtio LANGUAGES C CXX)

set(target_name virtio_net)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog CONFIG REQUIRED)

set(include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(src_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)


set(sources
    ${src_dir}/vfio_handler.cpp
    ${src_dir}/pci_device.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)


add_executable(${target_name} ${sources})
target_include_directories(${target_name} PUBLIC ${include_dir})
target_link_libraries(${target_name} spdlog::spdlog)