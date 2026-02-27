#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vfio.h>
#include <cstring>
#include <vector>
#include <sys/mman.h>

// Group 6 and Device 01:00.0 based on your last setup
#define IOMMU_GROUP_NUM 6
#define DEVICE_BDF "0000:01:00.0"

struct pci_header 
{
    uint16_t vendor_id;
    uint16_t device_id;
};

class PciDevice
{
public:
    PciDevice(const std::string& bdf, int iommu_group)
    {
        // Step 1: Open Container
        container_fd_ = open("/dev/vfio/vfio", O_RDWR);
        if (container_fd_ < 0) 
        {
            throw std::runtime_error("Failed to open VFIO container");
        }

        if (ioctl(container_fd_, VFIO_GET_API_VERSION) != VFIO_API_VERSION) 
        {
            throw std::runtime_error("VFIO API version mismatch");
        }
        
        if (!ioctl(container_fd_, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) 
        {
            throw std::runtime_error("IOMMU type not supported");
        }

        // Step 2: Open Group
        const std::string group_path = "/dev/vfio/" + std::to_string(iommu_group);
        group_fd_ = open(group_path.c_str(), O_RDWR);
        if (group_fd_ < 0) 
        {
            throw std::runtime_error("Failed to open IOMMU group file");
        }

        // Step 3: Add Group to Container
        if (ioctl(group_fd_, VFIO_GROUP_SET_CONTAINER, &container_fd_) < 0) 
        {
            throw std::runtime_error("Failed to set group container");
        }

        // Step 4: Enable the IOMMU model we want
        if (ioctl(container_fd_, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU) < 0) 
        {
            throw std::runtime_error("Failed to set IOMMU type");
        }

        // Step 5: Get Device FD
        device_fd_ = ioctl(group_fd_, VFIO_GROUP_GET_DEVICE_FD, bdf.c_str());
        if (device_fd_ < 0) 
        {
            throw std::runtime_error("Failed to get device FD");
        }

        std::cout << "SUCCESS: Device opened! FD: " << device_fd_ << std::endl;
    }

    void read_config_space()
    {
        // Step 6: Find Config Space Region Info
        
        vfio_region_info reg = { .argsz = sizeof(reg) };
        reg.index = VFIO_PCI_CONFIG_REGION_INDEX;

        if (ioctl(device_fd_, VFIO_DEVICE_GET_REGION_INFO, &reg) < 0) 
        {
            throw std::runtime_error("Failed to get config region info");
        }

        std::cout << "Config Space found at offset: 0x" << std::hex << reg.offset 
                << " (Size: 0x" << reg.size << ")" << std::endl;

        // Step 7: Read PCI Config Space 
        pci_header header;

        // Now we read from reg.offset, NOT from 0
        ssize_t ret = pread(device_fd_, &header, sizeof(header), reg.offset);
        
        if (ret != sizeof(header)) 
        {
            throw std::runtime_error("Failed to read full PCI header");
        }

        std::cout << "Vendor ID: 0x" << std::hex << header.vendor_id << std::endl;
        std::cout << "Device ID: 0x" << std::hex << header.device_id << std::endl;

        if (header.vendor_id == 0x1af4 && header.device_id == 0x1041) 
        {
            std::cout << "MATCH! We are talking to the VirtIO Net device." << std::endl;
        } 
        else
        {
            std::cout << "Warning: IDs do not match expected VirtIO values." << std::endl;
        }
    }

    int map_bar_regions() {
        struct vfio_region_info reg = { .argsz = sizeof(reg) };
        
        // VirtIO devices typically use BARs 0-5. Let's try to map them.
        for (int i = 0; i < VFIO_PCI_BAR5_REGION_INDEX; i++) {
            reg.index = i;
            
            // 1. Get region information from VFIO
            if (ioctl(device_fd_, VFIO_DEVICE_GET_REGION_INFO, &reg) < 0) {
                perror("Failed to get region info");
                return -1;
            }

            // Check if the region is present and supports MMAP
            if (reg.size > 0 && (reg.flags & VFIO_REGION_INFO_FLAG_MMAP)) {
                printf("Mapping BAR %d, Size: 0x%llx, Offset: 0x%llx\n", 
                    i, reg.size, reg.offset);

                // 2. Map the region into user space memory
                void *map = mmap(NULL, reg.size, PROT_READ | PROT_WRITE, 
                                MAP_SHARED, device_fd_, reg.offset);
                                
                if (map == MAP_FAILED) {
                    perror("mmap failed");
                    return -1;
                }

                // TODO: Save 'map' pointer somewhere accessible. 
                // You will need this base address later to calculate 
                // pointers to specific VirtIO structures found in the capability list.
                
                // Example: region_base_addrs[i] = map;
            }
        }

        return 0;
    }

    ~PciDevice()
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

private:
    int container_fd_, group_fd_, device_fd_;
};


int main() 
{
    PciDevice pci_dev(DEVICE_BDF, IOMMU_GROUP_NUM);
    pci_dev.read_config_space();
    pci_dev.map_bar_regions();

    return 0;
}
