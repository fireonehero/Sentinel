# Sentinel

A network traffic analyzer with intrusion detection, written in C++20 on top of [libpcap](https://www.tcpdump.org/).

Sentinel captures packets live from a network interface or reads them from a saved `.pcap` file, and prints each packet's timestamp and size. The first packet is also shown as a hex dump.

> **Status:** early development. Capture and file reading work; packet parsing and detection are not implemented yet. See [Roadmap](#roadmap).

## Features

- Lists all capture interfaces on the machine
- Live capture from `wlan0`, running until Ctrl+C
- Offline reading of `.pcap` files (no root needed)
- Rejects anything that isn't an Ethernet link type (`DLT_EN10MB`)
- Per-packet timestamp (`seconds.microseconds`), captured length, and original length
- Hex/binary/ASCII dump of the first packet
- Graceful shutdown on Ctrl+C or `SIGTERM`: prints a summary and closes the capture; a second Ctrl+C force-quits

## Requirements

- Linux
- A C++20 compiler (GCC or Clang)
- CMake 3.20 or newer
- libpcap 1.10 or newer (development headers included)

On Arch / CachyOS:

```sh
sudo pacman -S base-devel cmake libpcap
```

On Debian / Ubuntu:

```sh
sudo apt install build-essential cmake libpcap-dev
```

## Building

```sh
cmake -S . -B build
cmake --build build
```

The binary is `build/sentinel`.

## Usage

**Read a saved capture** (no special permissions needed):

```sh
./build/sentinel samples/test.pcap
```

**Capture live** from `wlan0` until you press Ctrl+C (needs root or capture capabilities):

```sh
sudo ./build/sentinel
```

**Make a test capture file** with tcpdump (stops after 50 packets):

```sh
sudo tcpdump -i wlan0 -w samples/test.pcap -c 50
```

### Example output

```
Sentinel is starting.

Opened samples/test.pcap for reading
Link type: 1 (EN10MB)

packet 1: ts=1762800123.482915 caplen=66 len=66
<hex dump of packet 1>
packet 2: ts=1762800123.483301 caplen=1514 len=1514
...

Captured 50 packets
```

When stopped with Ctrl+C, `Interrupted` is printed before the summary, and the program still exits with status 0.

### Running without sudo

Instead of running the whole program as root, you can give the binary only the capabilities it needs:

```sh
sudo setcap cap_net_raw,cap_net_admin=eip build/sentinel
./build/sentinel
```

Rebuilding replaces the binary, so this has to be redone after each build.

## Project layout

```
Sentinel/
├── CMakeLists.txt
├── include/
│   ├── capture.hpp     # capture API: list, open live/offline, packet callback
│   └── output.hpp      # hex dump formatting
├── src/
│   ├── main.cpp        # startup, mode selection, signal handling, capture loop
│   ├── capture.cpp
│   └── output.cpp
├── samples/            # local .pcap files (git-ignored)
└── docs/
    ├── architecture.md
    └── development.md
```

## Documentation

- [Architecture](docs/architecture.md): how the program is structured and how a packet flows through it
- [Development](docs/development.md): building, testing, and debugging notes

## Roadmap

- [x] List interfaces, open a live capture, check the link type
- [x] Per-packet callback with timestamp and lengths
- [x] Read saved `.pcap` files
- [x] Graceful shutdown on Ctrl+C
- [ ] Parse Ethernet frames
- [ ] Parse higher-layer protocol headers
- [ ] Intrusion detection
- [ ] Choose the interface from the command line instead of hardcoding `wlan0`

## A note on privacy

Packet captures contain real data from your network: MAC and IP addresses, hostnames, and any unencrypted traffic. `.pcap` files are git-ignored for that reason. Only capture on networks you own or have permission to monitor.
