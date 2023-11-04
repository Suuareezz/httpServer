# Concurrent TCP Echo Server with Keep-Alive

This is a concurrent TCP echo server implemented in C, utilizing threads to handle multiple client connections simultaneously. The server provides basic support for serving files and handling HTTP requests.

## Prerequisites

Before running the server, ensure you have the following dependencies installed:

- C compiler (e.g., GCC)
- POSIX Threads (Pthreads) library

## Compilation

Compile the code using your C compiler. For example, using GCC:

```bash
gcc tcpechosrv.c -o tcpechosrv -lpthread
```

## Usage

To run the server, specify the port number as a command-line argument:

```bash
./tcpechosrv <port>
```

The server will listen on the specified port and accept incoming connections.

## Features

- Concurrent handling of multiple client connections using threads.
- Basic support for serving files over HTTP.
- Keep-alive mechanism to maintain connections.
- Support for POST Method.

## Configuration

- The default timeout for a keep-alive connection is set to 10 seconds.

## File Structure

- `tcpechosrv.c`: The main source code file for the server.
- `www/`: This directory should contain the web files to be served by the server.
- `README.md`: This file, providing information about the server and how to use it.

## Usage Examples

1. Start the server on port 8080:

```bash
./tcpechosrv 8080
```

2. Connect to the server using a web browser or an HTTP client.

3. The server will respond to incoming connections, echoing received data, and serving files from the `www/` directory.

4. To test the keep-alive feature, you can send multiple requests to the server with the "Connection: Keep-alive" header within the specified timeout (default: 10 seconds).

5. The server will reset the timeout for each new request within the specified time frame.

6. If no new request is received within the timeout, the server will close the connection.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

This code is a basic example of a concurrent TCP echo server. It can be extended and customized for specific use cases and requirements.

Feel free to modify and improve the code to meet your needs.