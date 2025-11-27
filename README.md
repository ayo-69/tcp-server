# C++ Boost.Asio TCP Chat Server

This project is a simple, multi-client TCP chat server implemented in C++ using the Boost.Asio library. It follows an asynchronous design, allowing it to handle multiple client connections efficiently within a single thread (or a thread pool).

## Features

-   Asynchronous handling of multiple clients.
-   Messages from one client are broadcast to all other connected clients.
-   Notifications when clients join or leave the chat.
-   Simple, text-based protocol.

## Requirements

To build and run this project, you will need:

-   A C++ compiler supporting C++11 (e.g., `g++`)
-   `make`
-   The Boost libraries installed (specifically, the header-only components of Asio are sufficient for this version of the code).

On Debian-based Linux systems (like Ubuntu), you can install the necessary dependencies with:
```bash
sudo apt-get update
sudo apt-get install build-essential libboost-dev
```

## Building the Server

A `Makefile` is provided for easy compilation.

1.  **Clean previous builds (optional):**
    ```bash
    make clean
    ```

2.  **Compile the server:**
    ```bash
    make
    ```
    This will create an executable file named `server` in the project directory.

## Running the Server

You can run the server with an optional port number. If no port is specified, it will default to `8080`.

**To run on the default port (8080):**
```bash
./server
```

**To run on a specific port (e.g., 9999):**
```bash
./server 9999
```
The server will print a message indicating that it is listening on the selected port.

## Connecting to the Server

You can connect to the chat server using a simple TCP client like `netcat` (`nc`).

1.  Open a new terminal window.
2.  Connect to the server using the same port it is running on.

**If the server is on port 8080:**
```bash
nc localhost 8080
```

**If the server is on port 9999:**
```bash
nc localhost 9999
```

You can open multiple terminal windows and run the `netcat` command in each to simulate multiple clients. Any message you type in one client's terminal will be broadcast to all other clients.
