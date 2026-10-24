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

- The device becomes a bus master. On conventional PCI/PCI-X it arbitrates for the shared bus (REQ#/GNT#); on PCIe, which is point-to-point, it simply issues memory read/write requests (TLPs).
- Once it is allowed to master the bus, it directly transfers data to/from system memory.
- No external DMA controller is required.
- This model applies to PCI, PCI-X, PCIe, NVMe, modern NICs, and GPUs.

**Examples:**
- PCIe SSD transferring blocks to RAM
- NIC placing packets directly into GPU memory (e.g., NVIDIA GPUDirect RDMA, which user-space stacks such as DPDK can make use of)
- GPU copying from host memory to device memory

### Third-Party DMA (Old Model)

In early PCs, the device itself did not perform the transfer.
Instead, an external DMA controller (like the Intel 8237A) performed the transfer for the device.

- The device requested a DMA channel.
- The external DMA controller moved the data.
- The device only signalled readiness (DREQ/DACK handshake); the CPU programmed the source/destination address, length and mode into the controller.

This architecture was common in ISA PCs.

## DMA Controller (Engine)

Whether embedded inside the device (first-party DMA) or external (third-party DMA), DMA controllers support several operating modes:

### Modes of DMA Operation

These are the classic shared-bus modes (the 8237 calls them block/demand and single-transfer modes); they describe how DMA shares the bus, and apply less directly to point-to-point PCIe.

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
- **Addressing Modes:** Incrementing, decrementing, fixed address.
- **Scatter-Gather:** A transfer made of multiple non-contiguous memory segments, described by a list of descriptors.
- **Interrupts:** Devices notify the CPU when DMA completes.
- **Register Access:** DMA engines are configured through registers: memory-mapped (MMIO) on modern devices, port I/O on the legacy 8237.

## ISA DMA (Old Architecture)

In early PCs (1980s–early 1990s), the Intel 8237A DMA controller handled DMA transfers.
This controller was implemented on the motherboard (later integrated into the southbridge/I/O controller).

**Key properties:**
- Only 24-bit addressing → 16MB memory limit
- 8 DMA channels from two cascaded 8237s, of which 7 are usable (channel 4 is the cascade); channels 0–3 are 8-bit, 5–7 are 16-bit
- Very slow (ISA operated at ~8 MHz)
- Typical ISA devices did not master the bus, data movement relied on the system DMA controller (ISA bus-master cards existed but were uncommon, and some early NICs used programmed I/O instead of DMA).
- CPU had to program the 8237 for every transfer

Devices like sound cards, floppy controllers, and early NICs relied on the system DMA controller to move data.

ISA DMA became obsolete when:

- PCI was introduced (1990s)
- Devices could master the bus themselves
- CPU architectures moved beyond 16MB addressing
- High bandwidth devices became common

By the early 2000s, ISA and third-party DMA were effectively dead.

Southbridge DMA engines remained only for legacy compatibility.

## Bus Mastering (Modern DMA)

Modern systems use bus mastering, where the device itself acts as a master on the PCI/PCIe bus.

**Key properties:**
- Devices can directly access system memory (first-party DMA).
- Devices gain access through the bus protocol (arbitration on shared PCI/PCI-X; directly issued read/write requests on point-to-point PCIe).
- Supports very high speeds (PCIe Gen5/Gen6).
- No external DMA controller needed.
- Devices like NICs, NVMe SSDs, GPUs all have internal DMA engines.

**Main advantages:**
- Scales with device speed – each device has its own DMA engine.
- CPU offloading - CPU only sets up descriptors, then the device handles the rest.
- Scatter-Gather – DMA engines can follow linked lists of memory segments.
- Parallelism – multiple devices concurrently transfer data.
- Peer-to-Peer DMA – PCIe devices can communicate without CPU or RAM involvement in the data path (GPU <-> NIC), provided the root complex/switches support routing between them.
- Works with cache-coherent DMA on modern platforms, and with an IOMMU, which translates and restricts the addresses a device may access.
