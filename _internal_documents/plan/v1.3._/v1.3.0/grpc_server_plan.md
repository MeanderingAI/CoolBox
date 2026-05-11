# trekker/IO/grpc_server — design plan

**Location:** `_libraries/groups/trekker/IO/grpc_server/`  
**Date:** May 9 2026  
**Status:** planned — not yet implemented

## Motivation

`http_server` covers HTTP/1.1. `quic_transport` covers HTTP/3. gRPC (HTTP/2 + Protocol Buffers) is the natural third transport in the IO group, enabling strongly-typed RPC between internal services and from any gRPC-capable client.

## Design constraints

- No external dependency on `grpc`, `protobuf`, or `abseil` — matches the philosophy of `http_server` (raw sockets, no libhttp)
- Same constructor shape as `HttpServer`: `GrpcServer(port, num_threads, Logger*, shared_ptr<GrpcServiceHandler>)`
- CMake target `grpc_server SHARED`, public deps: `advanced_logging`, `thread_pool_utils`, `ws2_32` (Windows)

## Library layout

```
grpc_server/
├── CMakeLists.txt
├── headers/
│   ├── grpc_server.h      # GrpcServer class
│   ├── grpc_frame.h       # HTTP/2 frame types + parser
│   └── grpc_codec.h       # GrpcCall, GrpcResponse, varint helpers
└── source/
    ├── grpc_server.cpp
    ├── grpc_frame.cpp
    └── grpc_codec.cpp
```

## Wire protocol (HTTP/2 + gRPC framing)

1. Accept TCP connection (same BSD socket accept loop as `http_server`)
2. Exchange HTTP/2 connection preface (`PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n`) and SETTINGS frames
3. Parse incoming binary frames: `DATA`, `HEADERS`, `SETTINGS`, `WINDOW_UPDATE`, `PING`, `GOAWAY`
4. HPACK static-table header decompression (no dynamic table needed for simple use)
5. Strip 5-byte gRPC data frame prefix (1-byte compressed flag + 4-byte big-endian message length)
6. Hand-rolled protobuf varint encode/decode in `grpc_codec.cpp`
7. Dispatch to handler, encode response struct back to protobuf bytes, wrap in DATA frame + trailers (`grpc-status`, `grpc-message`)

## Handler interface

```cpp
namespace io::grpc_server {

struct GrpcCall {
    std::string service;       // e.g. "cluster.TopologyService"
    std::string method;        // e.g. "GetTopology"
    std::string request_body;  // raw protobuf bytes after 5-byte prefix
    std::map<std::string, std::string> metadata;
};

struct GrpcResponse {
    int         grpc_status = 0;  // 0 = OK; matches grpc-status trailer
    std::string body;             // raw protobuf bytes to send
    std::string error_message;    // sent as grpc-message trailer on non-zero status
};

class GrpcServiceHandler {
public:
    virtual ~GrpcServiceHandler() = default;
    virtual GrpcResponse handle(const GrpcCall& call) = 0;
};

class GrpcServer {
public:
    GrpcServer(int port, size_t num_threads,
               advanced_logging::Logger* logger,
               std::shared_ptr<GrpcServiceHandler> handler);
    void start();
    void stop();
};

} // namespace io::grpc_server
```

## Planned usage in distribution_tag_master

Expose `cluster.TopologyService/GetTopology` on port **7701** alongside the existing HTTP endpoint on 7700:

```cpp
auto grpc_handler = std::make_shared<TopologyGrpcHandler>(tstate);
io::grpc_server::GrpcServer grpc_srv(7701, 2, &topo_log, grpc_handler);
std::thread grpc_thread([&]{ grpc_srv.start(); });
// ...
grpc_srv.stop(); grpc_thread.join();
```

`TopologyGrpcHandler` routes on `call.service + call.method`, encodes the same cluster data as `TopologyServlet` but into protobuf bytes instead of JSON.

## Key implementation complexity

The hardest part is `grpc_frame.cpp` — specifically HPACK static-table lookup and the CONTINUATION frame reassembly state machine. Protobuf varint encoding and the server accept loop are straightforward.
