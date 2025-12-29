cmake_minimum_required(VERSION 3.10)

project(MyVirtioNet LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(virtio_net read_pci_config.cpp)
