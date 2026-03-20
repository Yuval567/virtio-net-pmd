#include "vfio_handler.h"

#include <linux/vfio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>


VfioHandle::VfioHandle(const std::string& bdf, int iommu_group)
    : container_fd_(-1), group_fd_(-1), device_fd_(-1)
{
    open_container();
    open_group(iommu_group);
    get_device_fd(bdf);

    std::cout << "SUCCESS: Device opened! FD: " << device_fd_ << std::endl;
}

VfioHandle::~VfioHandle()
{
    if (device_fd_ >= 0)
    {
        close(device_fd_);
    }
    if (group_fd_ >= 0)
    {
        close(group_fd_);
    }
    if (container_fd_ >= 0)
    {
        close(container_fd_);
    }
}

void VfioHandle::open_container()
{
    container_fd_ = open("/dev/vfio/vfio", O_RDWR);
    if (container_fd_ < 0)
    {
        throw std::runtime_error("Failed to open VFIO container");
    }

    // Check API version
    if (ioctl(container_fd_, VFIO_GET_API_VERSION) != VFIO_API_VERSION)
    {
        throw std::runtime_error("VFIO API version mismatch");
    }

    // Check if the IOMMU type we want is supported
    if (!ioctl(container_fd_, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU))
    {
        throw std::runtime_error("IOMMU type not supported");
    }
}

void VfioHandle::open_group(int iommu_group)
{
    const std::string group_path = "/dev/vfio/" + std::to_string(iommu_group);
    group_fd_ = open(group_path.c_str(), O_RDWR);
    if (group_fd_ < 0)
    {
        throw std::runtime_error("Failed to open IOMMU group file");
    }

    // Add Group to Container
    if (ioctl(group_fd_, VFIO_GROUP_SET_CONTAINER, &container_fd_) < 0)
    {
        throw std::runtime_error("Failed to set group container");
    }

    // Enable the IOMMU model we want
    if (ioctl(container_fd_, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU) < 0)
    {
        throw std::runtime_error("Failed to set IOMMU type");
    }

}

void VfioHandle::get_device_fd(const std::string& bdf)
{
    device_fd_ = ioctl(group_fd_, VFIO_GROUP_GET_DEVICE_FD, bdf.c_str());
    if (device_fd_ < 0)
    {
        throw std::runtime_error("Failed to get device FD");
    }
}