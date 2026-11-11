#ifndef CAPTURE_HPP
#define CAPTURE_HPP

#include <pcap/pcap.h>

bool list_interfaces();

pcap_t *open_live(const char *dev);

pcap_t *open_offline(const char *path);

void packet_handler(u_char *user, const pcap_pkthdr *header, const u_char *bytes);

#endif
