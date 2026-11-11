# Architecture

This document describes how Sentinel is structured and how a packet travels through it.

## Modules

| File | Responsibility |
|---|---|
| `src/main.cpp` | Program entry point. Initializes libpcap, chooses live or offline mode, checks the link type, installs the signal handler, runs the capture loop, prints the summary, and cleans up. |
| `include/capture.hpp`, `src/capture.cpp` | Everything that talks to a capture source: listing interfaces, opening a live or offline handle, and the per-packet callback. |
| `include/output.hpp`, `src/output.cpp` | Formatting only. `output()` turns a byte buffer into a binary + hex + ASCII dump, 16 bytes per line. It has no libpcap dependency. |

### `capture` API

```cpp
bool    list_interfaces();
pcap_t *open_live(const char *dev);
pcap_t *open_offline(const char *path);
void    packet_handler(u_char *user, const pcap_pkthdr *header, const u_char *bytes);
```

- `list_interfaces()` calls `pcap_findalldevs`, prints each device's name and description, and frees the list. It returns `false` if the call fails or no devices are visible (usually a permissions problem).
- `open_live()` wraps `pcap_open_live` with fixed settings: snapshot length 65535 (whole packets), promiscuous mode off (many Wi-Fi drivers don't support it), and a 1000 ms read timeout. It prints any warning libpcap reports even when the open succeeds.
- `open_offline()` wraps `pcap_open_offline`.
- Both `open_*` functions print their own error and return `nullptr` on failure, so the caller only has to check for null.
- `packet_handler()` is the `pcap_loop` callback. See [Per-packet callback](#per-packet-callback).

## Program flow

```
main
 ├─ pcap_init(PCAP_CHAR_ENC_UTF_8)
 ├─ argv[1] given?
 │    ├─ yes → open_offline(argv[1])          packet_limit = -1
 │    └─ no  → list_interfaces()
 │             open_live("wlan0")             packet_limit = -1
 ├─ pcap_datalink() == DLT_EN10MB?  ── no → close, exit 1
 ├─ install SIGINT / SIGTERM handler
 ├─ pcap_loop(handle, packet_limit, packet_handler, &count)
 │     └─ libpcap calls packet_handler once per packet
 ├─ print "Interrupted" if stopped by Ctrl+C
 ├─ print "Captured N packets"
 └─ pcap_close()
```

Everything after the handle is opened is shared by both modes. That is why opening is split into `open_live` / `open_offline`: the only difference between the modes is how the `pcap_t *` is obtained.

The interface list is printed only in live mode. Without root, `pcap_findalldevs` can return an empty list, which would otherwise stop file reading, a mode that needs no special permissions.

## Link-type check

`pcap_datalink()` reports what kind of header every packet starts with. Sentinel only accepts `DLT_EN10MB` (value 1), meaning packets look like Ethernet frames. On Linux, Wi-Fi interfaces in normal (non-monitor) mode also report `DLT_EN10MB`. Rejecting anything else up front prevents an Ethernet parser from misreading packets with a different header layout.

## Per-packet callback

`pcap_loop` calls `packet_handler` once per packet with three arguments:

| Parameter | Contents |
|---|---|
| `user` | The pointer passed as the last argument to `pcap_loop`. Sentinel passes the address of an `int` counter owned by `main`. |
| `header` | `ts` (timestamp: `tv_sec` + `tv_usec`), `caplen` (bytes actually captured), `len` (original packet length) |
| `bytes` | The raw packet data, `caplen` bytes long |

### Why the counter goes through `user`

The counter lives in `main` and is passed in as `user`, not kept as a `static` variable inside the callback. This keeps the state visible to `main` (for the end-of-run summary) and is the same mechanism that later stages will use to pass detection state into the callback.

### `caplen` vs `len`

`len` is the packet's real size on the wire; `caplen` is how many bytes are in the buffer. They differ only when a packet is larger than the snapshot length. Anything that reads packet bytes must use `caplen` as the bound. The hex dump builds its buffer as `std::vector<uint8_t>(bytes, bytes + caplen)` for this reason.

## Signal handling

Without a handler, Ctrl+C (`SIGINT`) kills the process immediately: the summary never prints and `pcap_close` never runs. Sentinel catches `SIGINT` and `SIGTERM` and stops the loop gracefully instead.

- **The handler only calls `pcap_breakloop`.** Most functions, including `std::cout`, are not safe to call from a signal handler. The libpcap documentation specifically allows `pcap_breakloop`.
- **A global handle (`g_handle`) is required.** A signal handler receives only the signal number, so it cannot get the handle through a parameter. `g_handle` is `static` to `main.cpp`.
- **The handler is installed late,** after the link-type check and right before `pcap_loop`. Early-exit paths therefore never involve it.
- **`g_handle` is cleared before every `pcap_close` after installation.** This guarantees a late signal can never call `pcap_breakloop` on freed memory (use-after-free).
- **`SA_RESETHAND`** makes the handler run once and then restore the default action. The first Ctrl+C stops gracefully, and a second one force-quits if the program is ever stuck.
- **`pcap_loop` then returns `PCAP_ERROR_BREAK`.** This is treated as a normal stop: the summary prints and the exit status is 0. Only `PCAP_ERROR` is a failure.

## Error handling conventions

- Every libpcap call that can fail is checked. Errors go to `std::cerr`; normal output goes to `std::cout`, so the two can be redirected separately.
- Before a handle exists, errors come from the caller's `errbuf`. After it exists, they come from `pcap_geterr(handle)`.
- Functions that open resources report their own errors and return a sentinel value (`false` / `nullptr`); `main` just exits.
- Every successfully opened handle is closed on every exit path.
