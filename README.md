# virtio-net-pmd

A user-space **VirtIO network driver** written in C++20. It takes a virtio-net PCI device over with VFIO and receives packets by polling, without involving the kernel network stack. It is a learning project in the style of a DPDK PMD.

> **Status:** receive path only. There is no TX queue, control queue or multiqueue support yet.

## Features

- VFIO setup: container, IOMMU group and device file descriptors, with DMA memory mapped through the IOMMU (Type1)
- PCI configuration-space parsing: capability list and BAR mapping
- VirtIO 1.x (modern) PCI transport: capability discovery, device reset, status handshake and feature negotiation
- Split virtqueue implementation: descriptor table, available ring and used ring, with doorbell notifications
- Polling RX queue: buffers come from a DMA-backed mbuf pool, and consumed descriptors are refilled automatically

## Documentation

- [DMA](docs/research/dma.md): how DMA works, ISA vs bus mastering
- [PCIe DMA data path](docs/research/PCIe/pcie-dma-data-path.md): Root Complex, IOMMU, IMC and the path a DMA write takes
- [VirtIO](docs/research/virtio.md): PCI transport, initialization, virtqueues and virtio-net

## Requirements

- Linux with VFIO and an IOMMU enabled (e.g. `intel_iommu=on` or `amd_iommu=on`; inside a VM, a virtual IOMMU)
- A virtio-net PCI device bound to `vfio-pci` (a modern device with ID `1af4:1041`)
- Root, or permissions on the device's `/dev/vfio/<group>` file
- C++20 compiler, CMake ≥ 3.10

## Build

```bash
./devtools/setup.sh
```

## Run

```bash
sudo ./build/virtio_net [pci-bdf]
```

## Try it in QEMU

The commands below are an example setup (q35 machine with a virtual IOMMU and a tap backend). Adjust paths and names to your system.

**Host:** start a guest with a virtual IOMMU and a virtio-net device behind it:

```bash
sudo ip tuntap add dev tap0 mode tap
sudo ip link set tap0 up

qemu-system-x86_64 -machine q35,accel=kvm -cpu host -m 2G \
    -device intel-iommu,intremap=on,caching-mode=on \
    -netdev tap,id=net1,ifname=tap0,script=no,downscript=no \
    -device virtio-net-pci,netdev=net1,disable-legacy=on,iommu_platform=on,ats=off \
    <your usual disk / console options for a Linux guest>
```

**Guest:** boot with `intel_iommu=on`, then bind the device to `vfio-pci` (find its address with `lspci -nn -d 1af4:1041`) and run the driver. The guest also needs an ordinary NIC for SSH/console, which you must leave alone:

```bash
BDF=0000:00:05.0
sudo modprobe vfio-pci
echo $BDF | sudo tee /sys/bus/pci/devices/$BDF/driver/unbind
echo 1af4 1041 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
sudo ./build/virtio_net $BDF
```

**Host:** send test frames to the device:

```bash
sudo python3 devtools/send_packets.py tap0 10
```


## License

[MIT](LICENSE)
