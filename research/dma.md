# Direct Memory Access (DMA)

## Introduction

Direct Memory Access (DMA) is a hardware capability that allows devices to transfer data to or from system memory **without involving the CPU for every byte**.

Instead of the CPU copying data manually (which is slow and wastes CPU cycles), a DMA engine performs memory transfers autonomously.

This improves performance, enables true parallelism, and reduces CPU load in systems such as:
- Disk controllers
- Network Interface Cards (NICs)
- GPUs
- Audio devices
- Any high-bandwidth I/O device

## First-Party DMA vs Third-Party DMA

### First-Party DMA (Modern DMA)

In first-party DMA, the device itself contains its own DMA engine.

- The device requests ownership of the bus.
- Once granted, it directly transfers data to/from system memory.
- No external DMA controller is required.
- This model applies to PCI, PCI-X, PCIe, NVMe, modern NICs, and GPUs.

**Examples:**
- PCIe SSD transferring blocks to RAM
- NIC placing packets directly into GPU memory (e.g., GPUDirect RDMA with a user-space stack like DPDK)
- GPU copying from host memory to device memory

### Third-Party DMA (Old Model)

In early PCs, the device itself did not perform the transfer.
Instead, an external DMA controller (like the Intel 8237A) performed the transfer for the device.

- The device requested a DMA channel.
- The external DMA controller moved the data.
- The device had only to supply the source/destination info.

This architecture was common in ISA PCs.

## DMA Controller (Engine)

Whether embedded inside the device (first-party DMA) or external (third-party DMA), DMA controllers support several operating modes:

### Modes of DMA Operation

#### Burst Mode
- The DMA engine transfers a large block of data in one operation.
- The DMA takes full ownership of the bus until the transfer finishes.
- Fastest, but other bus agents may starve temporarily.

#### Cycle-Stealing Mode
- DMA performs small transfers, releasing the bus frequently.
- CPU and DMA share bus cycles.
- Slower, but allows more parallelism.

#### Transparent DMA
- DMA runs only when the CPU is idle.
- Useful for low-priority transfers.
- No interference with CPU execution.

## General DMA Concepts

- **DMA Channels:** Independent logical DMA pipelines that can perform transfers in parallel or be configured separately.
- **Addressing Modes:** Incrementing, decrementing, fixed address, scatter-gather.
- **Interrupts:** Devices notify the CPU when DMA completes.
- **Memory Mapped I/O:** DMA engines are configured using registers mapped into the CPU’s physical address space.

## ISA DMA (Old Architecture)

In early PCs (1980s–early 1990s), the Intel 8237A DMA controller handled DMA transfers.
This controller was implemented on the motherboard (later integrated into the southbridge/I/O controller)

**Key properties:**
- Only 24-bit addressing → 16MB memory limit
- Up to 8 DMA channels (some 8-bit, some 16-bit)
- Very slow (ISA operated at ~8 MHz)
- Typical ISA devices did not master the bus, data movement relied on the system DMA controller.
- CPU had to program the 8237 for every transfer

Devices like sound cards, floppy controllers, and early NICs relied on the system DMA controller to move data.

ISA DMA became obsolete when:

- PCI was introduced (1990s)
- Devices could master the bus themselves
- CPU architectures moved beyond 16MB addressing
- High bandwidth devices became common
- By the early 2000s, ISA and third-party DMA were effectively dead.

Southbridge DMA engines remained only for legacy compatibility.

## Bus Mastering (Modern DMA)

Modern systems use bus mastering, where the device itself acts as a master on the PCI/PCIe bus.

**Key properties:**
- Devices can directly access system memory (first-party DMA).
- Devices arbitrate for bus ownership through the bus protocol.
- Supports very high speeds (PCIe Gen5/Gen6).
- No external DMA controller needed.
- Devices like NICs, NVMe SSDs, GPUs all have internal DMA engines.

**Main advantages:**
- Scales with device speed – each device has its own DMA engine.
- CPU offloading - CPU only sets up descriptors, then the device handles the rest.
- Scatter-Gather – DMA engines can follow linked lists of memory segments.
- Parallelism – multiple devices concurrently transfer data.
- Peer-to-Peer DMA – PCIe devices can communicate without CPU or RAM involvement (GPU <-> NIC).
- Enables modern coherency models and IOMMU protection
