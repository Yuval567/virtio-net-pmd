#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/vfio.h>

// Group 6 and Device 01:00.0 based on your last setup
#define IOMMU_GROUP_NUM 6
#define DEVICE_BDF "0000:01:00.0"

// Minimal Type 0 PCI header layout (offset 0x00 - 0x3F)
// This matches the PCI specification for endpoint devices.
struct PciType0Header
{
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t  revision_id;
    uint8_t  prog_if;
    uint8_t  subclass;
    uint8_t  class_code;
    uint8_t  cache_line_size;
    uint8_t  latency_timer;
    uint8_t  header_type;
    uint8_t  bist;
    uint32_t bar[6];
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_addr;
    uint8_t  capabilities_ptr;
    uint8_t  reserved1[3];
    uint32_t reserved2;
    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint8_t  min_grant;
    uint8_t  max_latency;
};

// Virtio PCI capability as defined by the Virtio spec
struct VirtioPciCap
{
    uint8_t cap_vndr;   // PCI_CAP_ID_VNDR (0x09)
    uint8_t cap_next;
    uint8_t cap_len;
    uint8_t cfg_type;   // VIRTIO_PCI_CAP_* type
    uint8_t bar;        // Which BAR this capability refers to
    uint8_t padding[3];
    uint32_t offset;    // Offset within the BAR
    uint32_t length;    // Length of the structure
};

enum class VirtioPciCfgType : uint8_t
{
    COMMON   = 1,
    NOTIFY   = 2,
    ISR      = 3,
    DEVICE   = 4,
    PCI_CFG  = 5
};

class PciDevice
{
public:
    PciDevice(const std::string& bdf, int iommu_group)
        : container_fd_(-1), group_fd_(-1), device_fd_(-1)
    {
        open_vfio_container();
        open_vfio_group(iommu_group);
        get_device_fd(bdf);
        init_config_region();
    }

    void dump_pci_header()
    {
        PciType0Header hdr{};
        read_config(0, &hdr, sizeof(hdr));

        std::cout << std::hex;
        std::cout << "Vendor ID: 0x" << hdr.vendor_id
                  << ", Device ID: 0x" << hdr.device_id << std::endl;
        std::cout << "Command: 0x" << hdr.command
                  << ", Status: 0x" << hdr.status << std::endl;
        std::cout << "Class: 0x" << static_cast<int>(hdr.class_code)
                  << ", Subclass: 0x" << static_cast<int>(hdr.subclass)
                  << ", ProgIF: 0x" << static_cast<int>(hdr.prog_if) << std::endl;
        std::cout << "Capabilities Pointer: 0x"
                  << static_cast<int>(hdr.capabilities_ptr) << std::endl;

        if (hdr.vendor_id == 0x1af4)
        {
            std::cout << "Detected VirtIO device (vendor 0x1af4)." << std::endl;
        }
    }

    void dump_capabilities()
    {
        // Check if capabilities list is present
        uint16_t status = read_config16(0x06);
        if (!(status & (1u << 4)))
        {
            std::cout << "Device does not advertise a capabilities list." << std::endl;
            return;
        }

        uint8_t cap_ptr = read_config8(0x34);
        if (cap_ptr < 0x40)
        {
            std::cout << "No valid capability pointer (0x" << std::hex
                      << static_cast<int>(cap_ptr) << ")." << std::endl;
            return;
        }

        std::cout << "Walking PCI capability list starting at 0x" << std::hex
                  << static_cast<int>(cap_ptr) << std::endl;

        // Basic loop protection in case of malformed capability pointers
        for (int i = 0; i < 64 && cap_ptr != 0; ++i)
        {
            uint8_t header[3] = {};
            read_config(cap_ptr, header, sizeof(header));

            uint8_t cap_id  = header[0];
            uint8_t cap_next = header[1];
            uint8_t cap_len = header[2];

            std::cout << "  Cap at 0x" << std::hex << static_cast<int>(cap_ptr)
                      << ": id=0x" << static_cast<int>(cap_id)
                      << ", next=0x" << static_cast<int>(cap_next)
                      << ", len=" << std::dec << static_cast<int>(cap_len)
                      << std::endl;

            if (cap_id == 0x09) // Vendor-specific; VirtIO uses this
            {
                VirtioPciCap vcap{};
                read_config(cap_ptr, &vcap, sizeof(vcap));

                std::cout << "    Virtio PCI cap: cfg_type="
                          << static_cast<int>(vcap.cfg_type)
                          << ", bar=" << static_cast<int>(vcap.bar)
                          << ", offset=0x" << std::hex << vcap.offset
                          << ", length=0x" << std::hex << vcap.length
                          << std::endl;
            }

            if (cap_next == cap_ptr)
            {
                std::cout << "  Stopping: capability next pointer loops to itself." << std::endl;
                break;
            }

            cap_ptr = cap_next;
        }
    }

    int map_bar_regions()
    {
        struct vfio_region_info reg = { .argsz = sizeof(reg) };

        // VirtIO devices typically use BARs 0-5. Let's try to map them.
        for (int i = VFIO_PCI_BAR0_REGION_INDEX;
             i <= VFIO_PCI_BAR5_REGION_INDEX; ++i)
        {
            reg.index = i;

            // 1. Get region information from VFIO
            if (ioctl(device_fd_, VFIO_DEVICE_GET_REGION_INFO, &reg) < 0)
            {
                perror("Failed to get region info");
                return -1;
            }

            // Check if the region is present and supports MMAP
            if (reg.size > 0 && (reg.flags & VFIO_REGION_INFO_FLAG_MMAP))
            {
                std::printf("Mapping BAR %d, Size: 0x%llx, Offset: 0x%llx\n",
                            i - VFIO_PCI_BAR0_REGION_INDEX,
                            static_cast<unsigned long long>(reg.size),
                            static_cast<unsigned long long>(reg.offset));

                void* map = mmap(nullptr, reg.size,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED, device_fd_, reg.offset);

                if (map == MAP_FAILED)
                {
                    perror("mmap failed");
                    return -1;
                }

                // For now just track the mapping; later we'll use it
                // to access the virtio common/notify/device configuration.
                bar_mappings_.push_back(map);
                bar_sizes_.push_back(reg.size);
            }
        }

        return 0;
    }

    ~PciDevice()
    {
        for (std::size_t i = 0; i < bar_mappings_.size(); ++i)
        {
            if (bar_mappings_[i])
            {
                munmap(bar_mappings_[i], bar_sizes_[i]);
            }
        }

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
    void open_vfio_container()
    {
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
    }

    void open_vfio_group(int iommu_group)
    {
        const std::string group_path = "/dev/vfio/" + std::to_string(iommu_group);
        group_fd_ = open(group_path.c_str(), O_RDWR);
        if (group_fd_ < 0)
        {
            throw std::runtime_error("Failed to open IOMMU group file");
        }

        if (ioctl(group_fd_, VFIO_GROUP_SET_CONTAINER, &container_fd_) < 0)
        {
            throw std::runtime_error("Failed to set group container");
        }

        if (ioctl(container_fd_, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU) < 0)
        {
            throw std::runtime_error("Failed to set IOMMU type");
        }
    }

    void get_device_fd(const std::string& bdf)
    {
        device_fd_ = ioctl(group_fd_, VFIO_GROUP_GET_DEVICE_FD, bdf.c_str());
        if (device_fd_ < 0)
        {
            throw std::runtime_error("Failed to get device FD");
        }

        std::cout << "SUCCESS: Device opened! FD: " << device_fd_ << std::endl;
    }

    void init_config_region()
    {
        config_region_.argsz = sizeof(config_region_);
        config_region_.index = VFIO_PCI_CONFIG_REGION_INDEX;

        if (ioctl(device_fd_, VFIO_DEVICE_GET_REGION_INFO, &config_region_) < 0)
        {
            throw std::runtime_error("Failed to get config region info");
        }

        std::cout << "Config Space: offset=0x" << std::hex
                  << config_region_.offset
                  << ", size=0x" << config_region_.size
                  << std::dec << std::endl;
    }

    void read_config(uint32_t offset, void* buf, std::size_t len)
    {
        if (offset + len > config_region_.size)
        {
            throw std::runtime_error("Config read out of bounds");
        }

        off_t pos = static_cast<off_t>(config_region_.offset + offset);
        ssize_t ret = pread(device_fd_, buf, len, pos);
        if (ret != static_cast<ssize_t>(len))
        {
            throw std::runtime_error("Short read from PCI config space");
        }
    }

    uint8_t read_config8(uint32_t offset)
    {
        uint8_t v = 0;
        read_config(offset, &v, sizeof(v));
        return v;
    }

    uint16_t read_config16(uint32_t offset)
    {
        uint16_t v = 0;
        read_config(offset, &v, sizeof(v));
        return v;
    }

    int container_fd_;
    int group_fd_;
    int device_fd_;

    vfio_region_info config_region_{};

    std::vector<void*> bar_mappings_;
    std::vector<std::size_t> bar_sizes_;
};


int main() 
{
    try
    {
        PciDevice pci_dev(DEVICE_BDF, IOMMU_GROUP_NUM);
        pci_dev.dump_pci_header();
        pci_dev.dump_capabilities();
        pci_dev.map_bar_regions();
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}


    int map_bar_regions()
    {
        struct vfio_region_info reg = { .argsz = sizeof(reg) };

        // VirtIO devices typically use BARs 0-5. Let's try to map them.
        for (int i = 0; i < VFIO_PCI_BAR5_REGION_INDEX; i++)
        {
            reg.index = i;

            // 1. Get region information from VFIO
            if (ioctl(device_fd_, VFIO_DEVICE_GET_REGION_INFO, &reg) < 0)
            {
                perror("Failed to get region info");
                return -1;
            }

            // Check if the region is present and supports MMAP
            if (reg.size > 0 && (reg.flags & VFIO_REGION_INFO_FLAG_MMAP))
            {
                printf("Mapping BAR %d, Size: 0x%llx, Offset: 0x%llx\n",
                    i, reg.size, reg.offset);

                // 2. Map the region into user space memory
                void* map = mmap(NULL, reg.size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, device_fd_, reg.offset);

                if (map == MAP_FAILED)
                {
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
