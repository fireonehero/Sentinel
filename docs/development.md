# Development

Notes for building, testing, and working on Sentinel.

## Build configuration

`CMakeLists.txt` sets:

- **C++20**, with compiler extensions off (`CMAKE_CXX_EXTENSIONS OFF`), so the code stays standard C++
- **`-Wall -Wextra -Wpedantic`**. The code should build with zero warnings.
- **`CMAKE_EXPORT_COMPILE_COMMANDS ON`**, which writes `build/compile_commands.json` for clangd and other editor tooling
- linking against `pcap`

Configure once, then rebuild as needed:

```sh
cmake -S . -B build
cmake --build build
```

When you add a new `.cpp` file, add it to `add_executable` in `CMakeLists.txt`. A missing entry shows up as an `undefined reference to ...` link error.

## Adding a module

Follow the pattern of `output` and `capture`:

1. Declarations go in `include/<name>.hpp`, with an include guard (`#ifndef NAME_HPP`).
2. Definitions go in `src/<name>.cpp`, which includes its own header.
3. Add `src/<name>.cpp` to `add_executable`.

## Testing

### With a capture file (no root)

Reading files needs no special permissions, so this is the quickest loop:

```sh
sudo tcpdump -i wlan0 -w samples/test.pcap -c 50   # once
./build/sentinel samples/test.pcap
```

Things to check:

- The link type is `1 (EN10MB)`.
- All packets in the file are read. tcpdump says how many it wrote, and the `Captured N packets` summary should match.
- `caplen` equals `len` for every packet (the snapshot length is 65535).
- A missing file prints `pcap_open_offline(...): No such file or directory` and exits with status 1.

### Live capture

```sh
sudo ./build/sentinel
```

Open a web page to generate traffic. Expect a mix of small packets (around 54–66 bytes, mostly TCP acknowledgments) and large ones near 1514 bytes (data). Press Ctrl+C. You should see `Interrupted`, then the summary, and `echo $?` should print `0`.

### Testing shutdown

| Action | Expected |
|---|---|
| One Ctrl+C | `Interrupted`, summary, exit status 0 |
| `kill <pid>` (SIGTERM) | Same as Ctrl+C |
| Two Ctrl+C | First stops gracefully; second force-quits (`SA_RESETHAND`) |

## Permissions

Live capture needs `CAP_NET_RAW` (and `CAP_NET_ADMIN` for some setups). Options:

- Run with `sudo`.
- Grant the capabilities to the binary: `sudo setcap cap_net_raw,cap_net_admin=eip build/sentinel`. This must be repeated after every rebuild, because the linker writes a new file.

Symptoms of missing permissions:

- `list_interfaces()` reports `No devices found (try sudo)`.
- `pcap_open_live(wlan0): ... Operation not permitted`.

## Debugging tips

- **`tcpdump -D`** lists interfaces the same way `pcap_findalldevs` does, which makes it a quick cross-check.
- **`tcpdump -r samples/test.pcap`** shows what is in a capture file, so you can compare it with Sentinel's output.
- **`ip -brief link`** shows interface names and whether they are up.
- If `pcap_loop` seems to hang in live mode, there is probably no traffic. Generate some (for example `ping 8.8.8.8`).

## Known limitations

- The live interface is hardcoded to `wlan0`.
- Only Ethernet link types (`DLT_EN10MB`) are supported.
- Only the first packet is hex-dumped. The dump includes a binary column, so each line is very wide.
- Packet contents are not parsed yet.
