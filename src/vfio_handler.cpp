#include "vfio_handler.h"
#include "utils.h"

#include <linux/vfio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <filesystem>

#include "spdlog/spdlog.h"

VfioHandle::VfioHandle(const std::string& bdf)
    : container_fd_(-1), group_fd_(-1), device_fd_(-1)
{
    const int iommu_group = get_iommu_group(bdf);

    open_container();
    open_group(iommu_group);
    get_device_fd(bdf);

    spdlog::info("SUCCESS: Device opened! FD: {}", device_fd_);
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
        LOG_AND_THROW("Failed to open VFIO container");
    }

    // Check API version
    if (ioctl(container_fd_, VFIO_GET_API_VERSION) != VFIO_API_VERSION)
    {
        LOG_AND_THROW("VFIO API version mismatch");
    }

    // Check if the IOMMU type we want is supported
    if (!ioctl(container_fd_, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU))
    {
        LOG_AND_THROW("IOMMU type not supported");
    }
}

void VfioHandle::open_group(int iommu_group)
{
    const std::string group_path = "/dev/vfio/" + std::to_string(iommu_group);
    group_fd_ = open(group_path.c_str(), O_RDWR);
    if (group_fd_ < 0)
    {
        LOG_AND_THROW("Failed to open IOMMU group file");
    }

    // Add Group to Container
    if (ioctl(group_fd_, VFIO_GROUP_SET_CONTAINER, &container_fd_) < 0)
    {
        LOG_AND_THROW("Failed to set group container");
    }

    // Enable the IOMMU model we want
    if (ioctl(container_fd_, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU) < 0)
    {
        LOG_AND_THROW("Failed to set IOMMU type");
    }
}

void VfioHandle::get_device_fd(const std::string& bdf)
{
    device_fd_ = ioctl(group_fd_, VFIO_GROUP_GET_DEVICE_FD, bdf.c_str());
    if (device_fd_ < 0)
    {
        LOG_AND_THROW("Failed to get device FD");
    }
}

int VfioHandle::get_iommu_group(const std::string& bdf)
{
    const std::string group_link = "/sys/bus/pci/devices/" + bdf + "/iommu_group";

    std::error_code ec;
    const std::filesystem::path target = std::filesystem::read_symlink(group_link, ec);
    if (ec)
    {
        LOG_AND_THROW("Failed to resolve IOMMU group for device " + bdf);
    }

    try
    {
        return std::stoi(target.filename().string());
    }
    catch (const std::exception&)
    {
        LOG_AND_THROW("Failed to parse IOMMU group number for device " + bdf);
    }
}