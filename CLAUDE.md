# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`virtio-net-pmd`: a user-space VirtIO network driver in C++20 (DPDK-PMD style, learning project). It takes over a virtio-net PCI device (modern, `1af4:1041`) via VFIO and polls for packets. **RX path only** — no TX, control queue or multiqueue. Linux-only at runtime (VFIO/IOMMU); development may happen on Windows, but it cannot be built or run there.

## Build & run

```bash
./devtools/setup.sh   # rm -rf build, conan install (spdlog), cmake configure + build, Debug
```

- Dependency is spdlog via Conan (`conanfile.txt`); CMake uses `find_package(spdlog CONFIG REQUIRED)`, so configure needs the Conan toolchain file (`build/conan_toolchain.cmake`).
- Rebuild after the first setup: `cmake --build build`. Output binary: `build/virtio_net`.
- Sources are listed explicitly in `CMakeLists.txt` — add new `.cpp` files there.
- There are no tests or linter config.
- Running needs root (or access to `/dev/vfio/<group>`) and a device bound to `vfio-pci`. The PCI address is an optional argv (`virtio_net [bdf]`, default `DEFAULT_DEVICE_BDF` in `main.cpp`). Ctrl-C/SIGTERM stops the poll loop; `VirtioDevice::stop()` (also run by its destructor) resets the device so it stops DMA before rings/buffers are unmapped.
- `devtools/send_packets.py <iface> <count>` (scapy) sends test frames from the host to the device's tap interface.

## Architecture

Layered, each layer owning the one below it; wiring happens in `main()` (`main.cpp`):

1. `vfio/VfioHandle` — opens the VFIO container/group/device fds, sets up Type1 IOMMU, and `allocate_dma_memory()` returns a `dma_memory_region` (virtual address + IOVA + size) mapped through the IOMMU. All device-visible memory must come from here; the device sees IOVAs, not virtual addresses.
2. `pcie/PCIDevice` — parses PCI config space and the capability list, maps BARs (`mapped_bar.h`).
3. `virtio/VirtioDevice` — virtio 1.x PCI transport: finds virtio vendor capabilities (common/notify/ISR/device cfg), reset, status handshake, feature negotiation, `create_virtqueue()`, and `start()` (sets DRIVER_OK — queues must be fully set up beforehand, as in `main()`).
4. `virtio/Virtqueue` — split virtqueue (descriptor table, avail ring, used ring; layout in `virtqueue_layout.h`) plus doorbell notification.
5. `virtio/RxQueue` — wraps a Virtqueue and an `MbufMempool`; `receive_burst()` polls the used ring, returns `mbuf*`s, and refills descriptors. Callers must return buffers with `MbufMempool::free()`.
6. `common/` — `MbufMempool` (carves a DMA region into fixed-size mbufs; `mbuf` carries both the virtual pointer and IOVA), `ring_buffer.h`, helpers.

Startup order in `run()` (`main.cpp`) matters: VFIO → PCI → VirtioDevice → create virtqueue → allocate mbuf DMA region and mempool → `RxQueue` (fills the ring) → `virtio_device.start()` → poll loop. Shutdown is the reverse: `virtio_device.stop()` first, then the queue/mempool, then the mbuf DMA region is freed.

## Docs

`docs/research/` holds background notes (DMA, PCIe DMA data path, VirtIO transport/virtqueues) — consult these for the spec-level reasoning behind the code.
