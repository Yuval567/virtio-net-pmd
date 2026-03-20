#pragma once

#include <string>


class VfioHandle
{
public:
    VfioHandle(const std::string& bdf, int iommu_group);
    ~VfioHandle();

    int get_device_fd() const noexcept { return device_fd_; }

private:
    void open_container();
    void open_group(int iommu_group);
    void get_device_fd(const std::string& bdf);

    int container_fd_;
    int group_fd_;
    int device_fd_;
};