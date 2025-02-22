# cpp-tcp

This is an example TCP server library for studying `kqueue` and async c++

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
