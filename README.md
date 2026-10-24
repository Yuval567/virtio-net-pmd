# virtio-net-pmd

A user-space **VirtIO network driver** written in C++20. It takes a virtio-net PCI device over with VFIO and receives packets by polling, without involving the kernel network stack. It is a learning project in the style of a DPDK poll-mode driver.

> **Status:** receive path only. There is no TX queue, control queue or multiqueue support yet.

## Features

- VFIO setup: container, IOMMU group and device file descriptors, with DMA memory mapped through the IOMMU (Type1)
- PCI configuration-space parsing: capability list and BAR mapping
- VirtIO 1.x (modern) PCI transport: capability discovery, device reset, status handshake and feature negotiation
- Split virtqueue implementation: descriptor table, available ring and used ring, with doorbell notifications
- Polling RX queue: buffers come from a DMA-backed mbuf pool, and consumed descriptors are refilled automatically
- Negotiates only `VIRTIO_F_VERSION_1` and `VIRTIO_F_ACCESS_PLATFORM`

## Documentation

- [DMA](docs/research/dma.md): how DMA works, ISA vs bus mastering
- [PCIe DMA data path](docs/research/PCIe/pcie-dma-data-path.md): Root Complex, IOMMU, IMC and the path a DMA write takes
- [VirtIO](docs/research/virtio.md): PCI transport, initialization, virtqueues and virtio-net

## Requirements

- Linux with VFIO and an IOMMU enabled (e.g. `intel_iommu=on` or `amd_iommu=on`; inside a VM, a virtual IOMMU)
- A virtio-net PCI device bound to `vfio-pci` (a modern device with ID `1af4:1041`)
- Root, or permissions on the device's `/dev/vfio/<group>` file
- C++20 compiler, CMake ≥ 3.10, [Conan](https://conan.io/) 2 (the only dependency is `spdlog`)
- Python 3 with [Scapy](https://scapy.net/) to run the test traffic script

## Build

```bash
./devtools/setup.sh
```

## License

[MIT](LICENSE)
