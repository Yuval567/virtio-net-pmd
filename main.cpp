#include "pcie/pci_device.h"
#include "virtio/rx_queue.h"
#include "vfio/vfio_handler.h"
#include "virtio/virtio_device.h"
#include "common/mbuf_mempool.h"

#include <signal.h>

#include <array>
#include <csignal>
#include <exception>
#include <string>
#include <string_view>

#include "spdlog/spdlog.h"

constexpr uint32_t RX_BURST_SIZE = 32;
constexpr uint16_t BUFFER_SIZE = 2048;
constexpr uint32_t ETH_HEADER_LEN = 14;

static volatile std::sig_atomic_t g_running = 1;

static void handle_stop_signal(int)
{
    g_running = 0;
}

static void install_signal_handlers()
{
    struct sigaction sa{};
    sa.sa_handler = handle_stop_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

static void log_packet(const mbuf& buf)
{
    const uint8_t* payload = buf.data();
    const uint32_t payload_len = buf.data_len;

    if (payload_len < ETH_HEADER_LEN)
    {
        spdlog::info("RX packet [iova=0x{:x}]: len={} bytes", buf.data_iova(), payload_len);
        return;
    }

    spdlog::info(
        "RX packet [iova=0x{:x}]: len={} bytes, dst={:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}, "
        "src={:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}, ethertype=0x{:04x}",
        buf.data_iova(), payload_len,
        payload[0], payload[1], payload[2], payload[3], payload[4], payload[5],
        payload[6], payload[7], payload[8], payload[9], payload[10], payload[11],
        static_cast<uint16_t>((payload[12] << 8) | payload[13]));
}

static void rx_loop(RxQueue& rx_queue, MbufMempool& mempool)
{
    std::array<mbuf*, RX_BURST_SIZE> burst;
    while (g_running)
    {
        const uint32_t num_received = rx_queue.receive_burst(burst.data(), burst.size());

        for (uint32_t i = 0; i < num_received; ++i)
        {
            log_packet(*burst[i]);
            mempool.free(burst[i]);
        }
    }
}

static int run(const std::string& bdf)
{
    VfioHandle vfio_handler{bdf};
    PCIDevice pci_device(vfio_handler.get_device_fd());
    VirtioDevice virtio_device(vfio_handler.get_device_fd(), pci_device, vfio_handler);

    std::unique_ptr<Virtqueue> rx_virtqueue = virtio_device.create_virtqueue();
    const uint32_t num_rx_mbufs = static_cast<uint32_t>(rx_virtqueue->queue_size) * 2;
    dma_memory_region rx_buffer_region = vfio_handler.allocate_dma_memory(static_cast<uint64_t>(num_rx_mbufs) * BUFFER_SIZE);

    {
        MbufMempool rx_mempool(rx_buffer_region, num_rx_mbufs, BUFFER_SIZE);
        RxQueue rx_queue(std::move(rx_virtqueue), rx_mempool);
        virtio_device.start();

        spdlog::info("Listening for incoming packets on {} (Ctrl-C to stop)...", bdf);
        rx_loop(rx_queue, rx_mempool);

        spdlog::info("Shutting down...");
        // Reset first so the device stops DMA before the rings and buffers are unmapped.
        virtio_device.stop();
    }

    vfio_handler.free_dma_memory(rx_buffer_region);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc > 2 || (argc == 2 && std::string_view(argv[1]).starts_with('-')))
    {
        spdlog::error("Usage: {} [pci-bdf]", argv[0]);
        return 2;
    }

    if (argc == 1)
    {
        throw std::runtime_error("No PCI BDF provided");
    }

    const std::string bdf = std::string(argv[1]);

    install_signal_handlers();

    try
    {
        return run(bdf);
    }
    catch (const std::exception& e)
    {
        spdlog::error("Fatal: {}", e.what());
        return 1;
    }
}
