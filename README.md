# http-c

A minimal HTTP/1.0 server written in C for Windows, built on Winsock2. Parses the request line, routes paths and serves static files.

## Features

- **HTTP/1.0**. One request per connection, every response ends with `Connection: close`.
- **Request-line parsing** with 400 / 404 / 500 status codes.
- **Route table**. First match wins, query strings ignored.
- **Zero-copy files**. Static files go out through `TransmitFile`.
- **No dependencies**. Winsock2 and the C runtime only.

## Routes

- `/` — hello
- `/about` — about
- `/file` — serves `index.html`

## Stack

- C11, Winsock2, Win32 file handles
- [clang](https://clang.llvm.org) and `make` from [MSYS2 UCRT64](https://www.msys2.org)

## Getting started

```bash
make          # build
make run      # build and run
./server.exe  # listens on http://127.0.0.1:7000
make clean
```

## Project structure

```
server.c     # sockets, request parsing, routing, responses
fs.c / fs.h  # file open + size via Win32 handles
index.html   # served at /file
Makefile
```

## License

MIT. See [LICENSE](LICENSE).
