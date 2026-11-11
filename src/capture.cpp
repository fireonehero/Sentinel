#include <pcap/pcap.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include "capture.hpp"
#include "output.hpp"

bool list_interfaces()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *devices;

    if (pcap_findalldevs(&devices, errbuf) == PCAP_ERROR) {
        std::cerr << "pcap_findalldevs: " << errbuf << '\n';
        return false;
    }

    if (devices == nullptr) {
        std::cerr << "No devices found (try sudo)\n";
        return false;
    }

    std::cout << "Available interfaces: \n";

    for (pcap_if_t *d = devices; d != nullptr; d = d->next) {
        std::cout << d->name << " - " << (d->description ? d->description : "(no description)") << '\n';
    }

    pcap_freealldevs(devices);
    return true;
}

pcap_t *open_live(const char *dev)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    errbuf[0] = '\0';

    pcap_t *handle = pcap_open_live(dev, 65535, 0, 1000, errbuf);

    if (handle == nullptr) {
        std::cerr << "pcap_open_live(" << dev << "): " << errbuf << '\n';
        return nullptr;
    }
    if (errbuf[0] != '\0') {
        std::cerr << "warning: " << errbuf << '\n';
    }

    std::cout << "\nOpened " << dev << " for capture\n";
    return handle;
}

pcap_t *open_offline(const char *path)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t *handle = pcap_open_offline(path, errbuf);

    if (handle == nullptr) {
        std::cerr << "pcap_open_offline(" << path << "): " << errbuf << '\n';
        return nullptr;
    }

    std::cout << "\nOpened " << path << " for reading\n";
    return handle;
}

void packet_handler(u_char *user, const pcap_pkthdr *header, const u_char *bytes)
{
    int *count = reinterpret_cast<int *>(user);
    ++(*count);
    std::cout << "packet " << *count << ": ts=" << header->ts.tv_sec << '.' << std::setw(6) << std::setfill('0') << header->ts.tv_usec << " caplen=" << header->caplen << " len=" << header->len << '\n';

    if (*count == 1) {
        std::vector<uint8_t> data(bytes, bytes + header->caplen);
        std::cout << output(data) << '\n';
    }
}
