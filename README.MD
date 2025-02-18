# cpp-tcp

This is an example TCP library for studying `kqueue`, async c++

## Requirements
- clang
- make
- a BSD compatible machine (MacOS or FreeBSD)

## Running the project
```sh
make
```

## Testing the server

Connect using netcat, and type away!

```
nc 127.0.0.1 8080
```

## TODO
- Add compatibility with linux using `epoll`
- Add an Error enum to better describe any failing syscall
- Add a more robust way to read from a connection, avoiding hard coded buffer arrays
- Add a more ergonomic way to interact with `kqueue` and/or `epoll`
