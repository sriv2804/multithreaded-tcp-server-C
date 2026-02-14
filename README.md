# Multithreaded TCP Server in C

This project implements a multithreaded TCP server in C using a producer–consumer queue. The main thread accepts client sockets and enqueues them into a circular buffer; a pool of worker threads dequeues and serves them. Synchronization uses a pthread mutex and a condition variable to avoid busy-waiting when the queue is empty.

## Components

- **server.c**: The main server implementation, handling client connections and threading.
- **cq.c / cq.h**: Implementation of a circular shared buffer used for managing connections.
- **logger.c**: A logging module that spawns a separate thread for logging operations. (Note: Currently not integrated with the main server, but can be used independently or integrated in the future.)

## Getting Started

To compile and run this project, you will need a C compiler (e.g., GCC).

### Compilation

Using Makefile:

```bash
make
./server
```

Clean build artifacts:

```bash
make clean
```

Alternative manual build:

```bash
gcc -Wall -pthread server.c cq.c -o server
```

### Running the Server

```bash
./server
```

### Running the Client (Example)

Navigate to the `client` directory and compile `client.c`:

```bash
cd client
gcc client.c -o client
./client
```

