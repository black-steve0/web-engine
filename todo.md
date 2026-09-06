# API and Runtime Implementation Plan

## 1. Goal

Turn the web server into a general-purpose web application server.

The server should be compiled once and then allow users to build applications around it without recompiling the server itself.

The server should support:

* Static files
* HTTP APIs
* All common HTTP methods
* Python applications
* C++ applications
* Other runtimes in the future
* Per-request application execution
* Persistent applications
* WebSockets
* Server-Sent Events
* HTTP streaming
* GraphQL
* Middleware
* Application lifecycle management
* Configuration-driven routing

The main architecture should separate the **web server** from the **application runtime**.

---

# 2. Core Architecture

The overall flow should become:

```
Client
   ↓
HTTP Server
   ↓
Router
   ↓
Route Type
   ├── Static File
   ├── API
   ├── WebSocket
   ├── SSE
   ├── GraphQL
   └── Streaming
          ↓
       Runtime
          ↓
   Application
```

The HTTP server should be responsible for HTTP-related work.

The application should be responsible for application logic.

Do not make the HTTP server understand the internal implementation of every programming language.

---

# 3. Create a Unified Request/Response Model

Before implementing individual API features, define the concept of a request and response internally.

A request should contain things such as:

* HTTP method
* URL
* Path
* Query parameters
* Headers
* Cookies
* Request body
* Remote address
* Protocol information

A response should contain:

* HTTP status
* Response headers
* Response body
* Optional streaming information
* Optional connection-control information

This becomes the common interface between the server and runtimes.

For example:

```
HTTP request
      ↓
Server request object
      ↓
Runtime
      ↓
Server response object
      ↓
HTTP response
```

The runtime should not need to know how the underlying HTTP library works.

---

# 4. Redesign Routing

The existing static routing system should remain compatible.

Add a separate API routing system rather than mixing API routes directly into the static-file routes.

Conceptually:

```
routes
    html
    css
    js
    img

api
    products
    users
    authentication
```

This makes the configuration easier to understand and allows the routing system to distinguish between static resources and applications.

---

# 5. HTTP Methods

Make HTTP methods first-class route properties.

Support at minimum:

* GET
* POST
* PUT
* PATCH
* DELETE
* OPTIONS
* HEAD

Potentially support:

* CONNECT
* TRACE

The router should match both:

```
method
```

and:

```
path
```

A GET request and POST request to the same path should therefore be able to have different handlers.

For example, conceptually:

```
GET  /api/products
POST /api/products
```

can point to the same application while performing different operations.

---

# 6. Route Configuration

Extend the existing configuration system with an API section.

Each API route should describe:

* Route name
* HTTP method
* URL path
* Runtime
* Execution mode
* Application entry point
* Optional middleware
* Optional configuration
* Optional timeout
* Optional authentication requirements

The configuration should describe **what should happen**, not contain application logic.

The application itself should live outside the server configuration.

---

# 7. Runtime Abstraction

Create a runtime layer.

The server should not directly contain special logic for every programming language.

Instead:

```
Router
   ↓
Runtime Manager
   ↓
Python Runtime
C++ Runtime
WASM Runtime
etc.
```

Each runtime should implement the same general concept:

```
start application
send request
receive response
stop application
```

This allows new languages/runtimes to be added later without redesigning the HTTP server.

---

# 8. Execution Modes

Support two primary execution modes.

## Mode 1: Execute Per Request

The server starts the application whenever a request arrives.

Lifecycle:

```
Request
   ↓
Start application
   ↓
Send request information
   ↓
Receive response
   ↓
Application exits
   ↓
Send HTTP response
```

Use this mode for:

* Small scripts
* Infrequently used endpoints
* Simple applications
* Applications that are not designed to remain running

Advantages:

* Simple
* Strong process isolation
* No long-running application state
* Application crashes are isolated from the server

Disadvantages:

* Process startup cost
* Interpreter startup cost
* Repeated imports/initialization
* Repeated database connections
* Poorer performance under heavy traffic

---

# 9. Persistent Runtime Mode

The second mode should keep an application running.

Lifecycle:

```
Server starts
   ↓
Start application
   ↓
Application remains running
   ↓
Request arrives
   ↓
Send request to application
   ↓
Receive response
   ↓
Send HTTP response
   ↓
Application remains running
```

This should be the preferred mode for production applications.

The application can keep:

* Database connections
* Caches
* Loaded modules
* Configuration
* Models
* Application state
* Connection pools

alive between requests.

---

# 10. Application Communication

Persistent applications need a communication protocol.

Do not make the server depend on Python-specific or C++-specific communication.

Create a generic runtime protocol.

Conceptually:

```
Server → Runtime

Request ID
HTTP method
Path
Query
Headers
Body
```

And:

```
Runtime → Server

Request ID
Status
Headers
Body
```

The request ID is important.

It allows the server to associate a runtime response with the correct HTTP request.

---

# 11. Choose IPC

For persistent applications, use inter-process communication rather than starting a process for every request.

Possible options include:

### Standard input/output

Simple and easy to implement.

Good for an initial prototype.

### Unix domain sockets

Better suited to a production implementation on Linux/macOS.

Advantages:

* Fast local communication
* No exposed network port
* Good process separation
* Can support multiple requests

### Named pipes

Useful for Windows compatibility.

The runtime layer should hide the actual IPC mechanism from the rest of the server.

The architecture should therefore look like:

```
Runtime Manager
      ↓
   IPC Layer
      ↓
Python/C++ Application
```

---

# 12. Runtime Lifecycle Management

The server should own the lifecycle of persistent applications.

When the server starts:

1. Read configuration.
2. Discover configured applications.
3. Validate application configuration.
4. Start persistent runtimes.
5. Wait until they are ready.
6. Begin accepting HTTP requests.

When the server is shutting down:

1. Stop accepting new requests.
2. Tell persistent applications to shut down.
3. Allow applications time to finish existing work.
4. Forcefully terminate applications that do not shut down.
5. Close IPC connections.
6. Shut down the HTTP server.

The server should therefore act as the parent/process manager for its application runtimes.

---

# 13. Application Health

Persistent runtimes should have a concept of readiness.

The server should be able to determine:

* Whether the application started successfully
* Whether it is ready to receive requests
* Whether communication is still working
* Whether the application crashed
* Whether the application stopped responding

If a runtime crashes, the server should detect it.

Optionally support automatic restart.

Possible lifecycle:

```
STARTING
   ↓
READY
   ↓
RUNNING
   ↓
STOPPING
   ↓
STOPPED
```

And:

```
RUNNING
   ↓
CRASHED
   ↓
RESTARTING
   ↓
RUNNING
```

---

# 14. Concurrency

Persistent runtimes must eventually support concurrent requests.

There are several possible designs.

### Single application process

Requests are processed sequentially.

Simple but limited.

### Application with internal concurrency

The runtime handles multiple requests itself.

More efficient but requires the application framework to be designed for concurrency.

### Multiple runtime workers

The server starts several copies of the application.

For example:

```
Runtime Worker 1
Runtime Worker 2
Runtime Worker 3
Runtime Worker 4
```

The server distributes requests between them.

This is a good long-term design because it allows applications that are not thread-safe to still handle multiple requests.

---

# 15. Python Runtime

Python should be implemented as one runtime type.

Two modes should be supported.

### Python exec mode

Start the Python application for every request.

### Python persistent mode

Start a Python worker when the server starts.

The worker remains alive and communicates with the server.

Do not require users to modify the server itself when creating Python APIs.

They should only need to provide their application and configuration.

---

# 16. C++ Runtime

Do not require users to recompile the web server every time they change an API.

Instead, C++ applications should eventually be compiled as dynamically loadable components.

Depending on the operating system:

* Linux: shared libraries
* Windows: dynamic-link libraries
* macOS: dynamic libraries

Conceptually:

```
Web Server
    ↓
C++ Runtime
    ↓
User Application Plugin
```

The server loads the application component at runtime.

This allows the server executable to remain unchanged while the user's application changes.

---

# 17. C++ Persistent Applications

A C++ application should ideally be loaded once.

Lifecycle:

```
Server starts
    ↓
Load C++ application
    ↓
Initialize application
    ↓
Application remains loaded
    ↓
Requests are delivered
    ↓
Server shuts down
    ↓
Application cleanup
    ↓
Unload application
```

This provides performance close to native server code without requiring users to modify the server itself.

---

# 18. Future Runtimes

The runtime abstraction should make it possible to add:

* Python
* C++
* WebAssembly
* JavaScript
* Ruby
* Lua
* Other languages

without changing the routing architecture.

For example:

```
runtime=python

runtime=cpp

runtime=wasm
```

The router should not care which runtime is being used.

---

# 19. JSON API Support

The server should provide convenient handling for JSON APIs.

The request system should be able to expose:

* Parsed JSON body
* Content type
* Query parameters
* Headers

The response system should make it easy for runtimes to return:

* JSON
* Plain text
* HTML
* Binary data

The server should not force every API to return JSON, but JSON should be a first-class API format.

---

# 20. Middleware

Introduce middleware between routing and application execution.

Conceptually:

```
Request
   ↓
Middleware 1
   ↓
Middleware 2
   ↓
Authentication
   ↓
Router
   ↓
Runtime
   ↓
Response
   ↓
Middleware
   ↓
Client
```

Potential middleware:

* Logging
* CORS
* Authentication
* Rate limiting
* Compression
* Request validation
* Security headers
* Error handling

Middleware should be configurable and reusable.

---

# 21. WebSockets

WebSockets should not be treated as ordinary HTTP request/response APIs.

Create a separate persistent connection model.

Lifecycle:

```
HTTP Upgrade
    ↓
WebSocket connection
    ↓
Runtime
    ↓
Messages
    ↕
Runtime
    ↓
Connection closed
```

The runtime should receive events such as:

* Connection opened
* Message received
* Connection closed
* Error

The runtime should be able to send messages without waiting for a new HTTP request.

This means the runtime protocol needs to support events, not just request/response pairs.

---

# 22. Server-Sent Events

SSE should use a persistent HTTP connection.

Lifecycle:

```
HTTP request
    ↓
Runtime
    ↓
Open connection
    ↓
Send event
    ↓
Send event
    ↓
Send event
    ↓
Connection closes
```

The runtime therefore needs the ability to keep a response open and write data incrementally.

---

# 23. HTTP Streaming

Support streaming request and response bodies.

This is important for:

* Large files
* Video
* Downloads
* Uploads
* AI responses
* Long-running operations

Do not require the entire body to exist in memory.

The runtime interface should eventually support:

```
request body chunk
response body chunk
```

rather than only:

```
entire request
entire response
```

---

# 24. GraphQL

GraphQL should be implemented as another application/protocol layer rather than being built directly into the HTTP router.

Conceptually:

```
POST /graphql
       ↓
   GraphQL Handler
       ↓
    Schema
       ↓
   Resolver
       ↓
   Runtime
```

The server can provide the GraphQL transport while the user's application provides the schema and business logic.

Later, GraphQL subscriptions can use the same persistent/event infrastructure as WebSockets.

---

# 25. Database Support

Do not make the HTTP server itself responsible for application database logic.

Instead, persistent runtimes should be able to maintain database connections.

For example:

```
Application Runtime
      ↓
Database Connection Pool
      ↓
   Database
```

This avoids opening and closing database connections for every request.

A runtime SDK can eventually provide convenient database APIs.

---

# 26. Configuration Reloading

The existing configuration system already reloads configuration periodically.

Keep this capability, but distinguish between:

### Safe reload

Things such as:

* Routes
* MIME types
* Static directories
* Middleware settings

can potentially be reloaded without restarting applications.

### Runtime reload

If an application's:

* Entry point
* Runtime
* Execution mode
* Worker count

changes, the server may need to restart that runtime.

The server should detect the difference and perform the appropriate lifecycle operation.

---

# 27. Error Handling

Define clear runtime errors.

Examples:

* Application failed to start
* Application crashed
* Application timed out
* Invalid runtime response
* Runtime communication failure
* Invalid route
* Application not ready
* Application exceeded request limits

These should produce appropriate HTTP responses without crashing the main server.

---

# 28. Timeouts

Every runtime request should eventually have limits.

Potential limits:

* Application startup timeout
* Request timeout
* Runtime shutdown timeout
* Maximum request body size
* Maximum response size
* Maximum number of workers
* Maximum number of persistent connections

This prevents one broken application from consuming server resources indefinitely.

---

# 29. Security

Treat user applications as separate processes whenever possible.

Do not allow an application to automatically:

* Control the server process
* Modify arbitrary server configuration
* Access unrelated applications
* Access arbitrary files
* Execute unrestricted commands

The runtime system should eventually have permissions/capabilities.

For example:

```
filesystem access
network access
database access
environment variables
```

can be controlled independently.

This becomes especially important if the server is intended to host applications belonging to different users.

---

# 30. Logging

Separate server logs from application logs.

For example:

```
Server
  └── server.log

Application
  ├── stdout
  └── stderr
```

The server should capture application output for persistent runtimes.

Eventually provide:

* Application name
* Runtime
* Request ID
* Process ID
* Log level
* Timestamp

This makes debugging significantly easier.

---

# 31. Request IDs

Every incoming request should receive a unique internal request ID.

Use that ID throughout:

```
HTTP Server
    ↓
Router
    ↓
Middleware
    ↓
Runtime
    ↓
Application
```

This allows a single request to be traced across the entire system.

It is particularly important for persistent runtimes where many requests may be communicating over the same connection.

---

# 32. Recommended Development Order

Do not implement everything simultaneously.

Build the system in stages.

## Phase 1 — Unified HTTP Request/Response

Create the internal request and response model.

Make sure every HTTP handler can use it.

---

## Phase 2 — API Router

Add:

* API routes
* HTTP methods
* Path matching
* Request bodies
* Query parameters
* Headers
* Responses

At this stage, use a simple internal handler.

Do not implement Python or C++ yet.

---

## Phase 3 — Runtime Interface

Create the abstraction between the server and applications.

Define:

* Start
* Stop
* Request
* Response
* Error
* Health/readiness

This is the most important architectural stage.

---

## Phase 4 — Exec Runtime

Implement the simplest runtime first.

Support:

```
mode=exec
```

The server starts the application, communicates with it, receives the result, and waits for the application to finish.

This proves that the runtime architecture works.

---

## Phase 5 — Persistent Runtime

Add:

```
mode=persistent
```

Implement:

* Application startup
* IPC
* Request IDs
* Responses
* Application shutdown
* Crash detection

At this point the server becomes a real application host.

---

## Phase 6 — Python Runtime

Implement Python on top of the runtime abstraction.

Support:

* Exec mode
* Persistent mode
* Multiple workers later

The server itself should not contain Python-specific routing logic.

---

## Phase 7 — C++ Runtime

Implement dynamically loaded C++ applications.

Start with:

* Application loading
* Initialization
* Request handling
* Cleanup
* Unloading

Later add multiple workers and hot reloading.

---

## Phase 8 — Middleware

Add the middleware pipeline.

Start with:

1. Logging
2. CORS
3. Authentication
4. Error handling

Add more later.

---

## Phase 9 — Streaming

Add:

* Streaming responses
* Streaming requests
* Large-file support
* Long-lived connections

This requires extending the runtime protocol beyond simple request/response messages.

---

## Phase 10 — WebSockets

Build the persistent connection/event infrastructure.

This infrastructure can later be reused by:

* WebSockets
* GraphQL subscriptions
* Other realtime protocols

---

## Phase 11 — SSE

Build SSE on top of the streaming infrastructure.

It should require less infrastructure than WebSockets.

---

## Phase 12 — GraphQL

Implement GraphQL as a protocol/application layer using the existing runtime system.

---

## Phase 13 — Runtime Management

Add production features:

* Worker processes
* Automatic restart
* Health checks
* Timeouts
* Graceful shutdown
* Resource limits
* Logging
* Runtime monitoring

---

# 33. Final Architecture

The intended final architecture should look approximately like this:

```
┌──────────────────────────────────────┐
│              Web Server              │
│                                      │
│  HTTP                                │
│   ↓                                  │
│  Middleware                          │
│   ↓                                  │
│  Router                              │
│   ├── Static Files                   │
│   ├── API                            │
│   ├── WebSocket                      │
│   ├── SSE                            │
│   ├── GraphQL                        │
│   └── Streaming                      │
│                                      │
│            ↓                         │
│       Runtime Manager                │
└────────────┬─────────────────────────┘
             │
   ┌─────────┼─────────┐
   ↓         ↓         ↓
Python      C++       WASM
Runtime    Runtime    Runtime
   │         │         │
   ↓         ↓         ↓
App       Plugin      App
```

The important boundary is:

```
Web Server ↔ Runtime
```

Once that interface is stable, the server executable can remain unchanged while users install, replace, or update their applications independently.

---

# 34. The Main Design Principle

The server should become the **host**, not the application.

Users should be able to:

1. Install the compiled server.
2. Create their application.
3. Configure routes.
4. Choose a runtime.
5. Choose `exec` or `persistent`.
6. Start the server.
7. Have the server manage the application's lifecycle.

The server should handle networking, routing, protocols, middleware, lifecycle management, and communication.

The user's runtime should handle application logic.

That separation is the foundation for supporting Python, C++, WebSockets, GraphQL, SSE, streaming, and future runtimes without repeatedly recompiling the web server.
