#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vfio.h>
#include <cstring>
#include <vector>

// Group 6 and Device 01:00.0 based on your last setup
#define IOMMU_GROUP_NUM 6
#define DEVICE_BDF "0000:01:00.0"

int main() {
    int container_fd, group_fd, device_fd;
    
    // --- Step 1: Open Container ---
    container_fd = open("/dev/vfio/vfio", O_RDWR);
    if (container_fd < 0) {
        perror("Failed to open /dev/vfio/vfio");
        return 1;
    }

    if (ioctl(container_fd, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
        std::cerr << "Unknown VFIO API version" << std::endl;
        return 1;
    }

    if (!ioctl(container_fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
        std::cerr << "Type1 IOMMU not supported" << std::endl;
        return 1;
    }

    // --- Step 2: Open Group ---
    std::string group_path = "/dev/vfio/" + std::to_string(IOMMU_GROUP_NUM);
    group_fd = open(group_path.c_str(), O_RDWR);
    if (group_fd < 0) {
        perror("Failed to open IOMMU group file");
        return 1;
    }

    // --- Step 3: Add Group to Container ---
    if (ioctl(group_fd, VFIO_GROUP_SET_CONTAINER, &container_fd) < 0) {
        perror("Failed to set group container");
        return 1;
    }

    // --- Step 4: Set IOMMU Type ---
    if (ioctl(container_fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU) < 0) {
        perror("Failed to set IOMMU type");
        return 1;
    }

    // --- Step 5: Get Device FD ---
    device_fd = ioctl(group_fd, VFIO_GROUP_GET_DEVICE_FD, DEVICE_BDF);
    if (device_fd < 0) {
        perror("Failed to get device FD");
        return 1;
    }
    
    std::cout << "SUCCESS: Device opened! FD: " << device_fd << std::endl;

    // --- Step 6: Find Config Space Region Info ---
    // This is the FIX! We must ask where the config space is.
    struct vfio_region_info reg = { .argsz = sizeof(reg) };
    reg.index = VFIO_PCI_CONFIG_REGION_INDEX;

    if (ioctl(device_fd, VFIO_DEVICE_GET_REGION_INFO, &reg) < 0) {
        perror("Failed to get config region info");
        return 1;
    }

    std::cout << "Config Space found at offset: 0x" << std::hex << reg.offset 
              << " (Size: 0x" << reg.size << ")" << std::endl;

    // --- Step 7: Read PCI Config Space ---
    struct {
        uint16_t vendor_id;
        uint16_t device_id;
    } __attribute__((packed)) pci_header;

    // Now we read from reg.offset, NOT from 0
    ssize_t ret = pread(device_fd, &pci_header, sizeof(pci_header), reg.offset);
    
    if (ret != sizeof(pci_header)) {
        perror("Failed to read PCI config space");
        return 1;
    }

    std::cout << "--- PCI CONFIG SPACE READ ---" << std::endl;
    std::cout << "Vendor ID: 0x" << std::hex << pci_header.vendor_id << std::endl;
    std::cout << "Device ID: 0x" << std::hex << pci_header.device_id << std::endl;

    if (pci_header.vendor_id == 0x1af4 && pci_header.device_id == 0x1041) {
        std::cout << "MATCH! We are talking to the VirtIO Net device." << std::endl;
    } else {
        std::cout << "Warning: IDs do not match expected VirtIO values." << std::endl;
    }

    close(device_fd);
    close(group_fd);
    close(container_fd);

    return 0;
}
