#include <pcap/pcap.h>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include "capture.hpp"

static pcap_t *g_handle = nullptr;

static void handle_signal(int /*signum*/)
{
    if (g_handle != nullptr) {
        pcap_breakloop(g_handle);
    }
}

int main(int argc, char *argv[]) {
    std::cout << "Sentinel is starting.\n";

    char errbuf[PCAP_ERRBUF_SIZE];
    errbuf[0] = '\0';

    if (pcap_init(PCAP_CHAR_ENC_UTF_8, errbuf) == PCAP_ERROR) {
        std::cerr << "pcap_init: " << errbuf << '\n';
        return EXIT_FAILURE;
    }

    const char *source;
    pcap_t *handle;
    int packet_limit;

    if (argc > 1) {
        source = argv[1];
        handle = open_offline(source);
        packet_limit = -1;
    } else {
        if (!list_interfaces()) {
            return EXIT_FAILURE;
        }
        source = "wlan0";
        handle = open_live(source);
        packet_limit = -1;
    }

    if (handle == nullptr) {
        return EXIT_FAILURE;
    }

    int linktype = pcap_datalink(handle);
    const char *linkname = pcap_datalink_val_to_name(linktype);
    std::cout << "Link type: " << linktype << " (" << (linkname ? linkname : "unknown") << ")\n";

    if (linktype != DLT_EN10MB) {
        std::cerr << source << ": only Ethernet link types are supported\n";
        pcap_close(handle);
        return EXIT_FAILURE;
    }

    g_handle = handle;

    struct sigaction sa {};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    if (sigaction(SIGINT, &sa, nullptr) == -1 || sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::perror("sigaction");
        g_handle = nullptr;
        pcap_close(handle);
        return EXIT_FAILURE;
    }

    std::cout << '\n';
    int count = 0;
    int rc = pcap_loop(handle, packet_limit, packet_handler, reinterpret_cast<u_char *>(&count));
    if (rc == PCAP_ERROR) {
        std::cerr << "pcap_loop: " << pcap_geterr(handle) << '\n';
        g_handle = nullptr;
        pcap_close(handle);
        return EXIT_FAILURE;
    }
    if (rc == PCAP_ERROR_BREAK) {
        std::cout << "\nInterrupted\n";
    }

    std::cout << "\nCaptured " << count << " packets\n";

    g_handle = nullptr;
    pcap_close(handle);

    return EXIT_SUCCESS;
}
