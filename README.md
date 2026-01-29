# BLIGHT

A lightweight C-based C2 framework for distributed command execution.

## Features

- TCP client-server model
- Default host/port configuration
- CRC32-based termination command
- Exit code reporting per command
- Client grouping (`cmb`)
- Bulk command dispatch
- Length-prefixed output handling

## Dependencies

- GCC
- zlib

## Build

- gcc -o client client.c ascii_art.c -lz
- gcc -o server server.c ascii_art.c -lz

## Run

### Server

./server [bind_host port]

### Client

./client [host port]

## Commands

- Execute any shell command on active clients.
- `cmb id1 id2 ...`: Target next command to specified clients.
- `disconnect`: Terminate targeted client connections.

## Disclaimer

!! For educational and authorized testing purposes only !!
