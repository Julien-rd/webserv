<div align="center">

# Webserv | @42Vienna

### HTTP/1.1 Web Server in C++98

A custom HTTP server written from scratch in **C++98** as part of the 42 curriculum.

The goal of this project is to understand how real web servers operate internally by implementing networking, request parsing, routing, configuration handling, event-driven I/O, and CGI execution without relying on external web frameworks.

---

**Status:** 🚧 In Development

</div>

---

# Table of Contents

- [About](#about)
- [Project Goals](#project-goals)
- [Current Features](#current-features)
- [Architecture Overview](#architecture-overview)
- [Repository Structure](#repository-structure)
- [HTTP Request Flow](#http-request-flow)
- [Configuration System](#configuration-system)
- [CGI Support](#cgi-support)
- [Build Instructions](#build-instructions)
- [Running the Server](#running-the-server)
- [Development Notes](#development-notes)
- [Future Work](#future-work)
- [42 Project Context](#42-project-context)

---

# About

Webserv is a learning-oriented implementation of an HTTP/1.1 server written entirely in C++98.

Rather than using existing web frameworks or networking libraries, the project focuses on implementing the core building blocks that power modern web servers:

- TCP socket communication
- Event-driven connection handling
- HTTP request parsing
- HTTP response generation
- Route matching
- Static file serving
- CGI execution
- Configuration parsing

The repository currently contains the foundation of these systems and serves as an experimental environment for building a complete Webserv implementation.

---

# Project Goals

The project aims to provide hands-on experience with:

- Low-level network programming
- Socket management
- Multiplexed I/O
- HTTP protocol internals
- Process creation and management
- Configuration parsing
- Resource management in C++98
- Designing large modular software projects

---

# Current Features

The repository currently includes implementations or prototypes for:

| Category | Status |
|----------|---------|
| Socket Management | 🚧 |
| HTTP Request Parsing | 🚧 |
| HTTP Response Generation | 🚧 |
| Configuration Parsing | 🚧 |
| Route Handling | 🚧 |
| CGI Execution | 🚧 |
| Client Management | 🚧 |
| Event Polling Layer | 🚧 |
| Static Content Testing | 🚧 |

> This project is under active development. Features may be incomplete, experimental, or subject to change.

---

# Architecture Overview

The codebase is organized into several independent components.

```text
Client
   │
   ▼
Server Socket
   │
   ▼
ServerManager
   │
   ├── Poller
   ├── Server
   ├── ConfigParser
   └── Client
          │
          ▼
     HttpRequest
          │
          ▼
      Routing
          │
    ┌─────┴─────┐
    ▼           ▼
Static Files   CGI
    │           │
    └─────┬─────┘
          ▼
    HttpResponse
          │
          ▼
       Client
```

Each subsystem is designed to remain relatively independent and focused on a specific responsibility.

---

# Repository Structure

```text
webserv/
│
├── CGI/
├── Client/
│   ├── HttpRequest/
│   └── HttpResponse/
│
├── ConfigParser/
├── Poller/
├── Server/
├── ServerManager/
├── URL_tool/
│
├── website/
├── tests/
│
├── Makefile
└── main.cpp
```

---

## Client

The Client subsystem is responsible for managing connected users and tracking request/response state throughout the lifetime of a connection.

Responsibilities include:

- Reading incoming data
- Buffer management
- Request lifecycle tracking
- Response transmission

---

## HttpRequest

The request parser is responsible for transforming raw TCP data into structured HTTP requests.

Typical responsibilities include:

- Parsing the request line
- Parsing headers
- Extracting the body
- Handling query strings
- Tracking request state

Example:

```http
GET /index.html HTTP/1.1
Host: localhost
Connection: keep-alive
```

---

## HttpResponse

The response subsystem builds valid HTTP responses from server-side data.

Responsibilities include:

- Status line generation
- Header generation
- Content-Length handling
- Response body delivery
- Error response generation

Example:

```http
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 123
```

---

## ConfigParser

The configuration parser reads server configuration files and converts them into internal configuration structures.

The long-term goal is to support an nginx-style configuration format.

Example:

```conf
server {
    listen 8080;
    server_name localhost;

    location / {
        root ./website;
    }
}
```

The parser is intended to provide:

- Server blocks
- Location blocks
- Root directories
- Error pages
- Method restrictions
- CGI configuration

---

## Poller

The Poller subsystem provides an abstraction around the chosen I/O multiplexing mechanism.

Its responsibilities include:

- Monitoring sockets
- Detecting readable connections
- Detecting writable connections
- Dispatching events

This component forms the foundation of the server's event-driven architecture.

---

## Server

The Server component manages:

- Listening sockets
- Network configuration
- Incoming connections
- Interaction with the polling system

---

## ServerManager

The ServerManager acts as the central coordinator of the application.

Responsibilities include:

- Initializing servers
- Managing active clients
- Processing events
- Dispatching requests
- Coordinating subsystem interaction

---

## CGI

The CGI subsystem enables execution of external scripts.

The intended workflow is:

```text
HTTP Request
      │
      ▼
Route Match
      │
      ▼
fork()
      │
      ▼
execve()
      │
      ▼
Capture Output
      │
      ▼
HTTP Response
```

CGI support allows dynamic content generation while keeping the web server itself relatively simple.

---

# HTTP Request Flow

A typical request passes through the following stages:

```text
accept()
    │
    ▼
Read Socket Data
    │
    ▼
HttpRequest Parser
    │
    ▼
Route Resolution
    │
    ▼
Handler Selection
    │
 ┌──┴──┐
 ▼     ▼
Static CGI
 │      │
 └──┬───┘
    ▼
HttpResponse
    │
    ▼
Write Socket
```

---

# Configuration System

The project contains a dedicated configuration parsing subsystem intended to support server customization without recompilation.

Configuration files are expected to control:

- Ports
- Hosts
- Virtual servers
- Routes
- CGI execution
- Static directories
- Error pages

---

# CGI Support

CGI allows the server to delegate dynamic content generation to external programs.

The CGI implementation is being developed around standard UNIX primitives:

- `fork()`
- `execve()`
- pipes
- environment variables
- process exit codes

Supported CGI languages may include:

- Python
- Bash
- Other executable scripts

depending on system configuration.

---

# Build Instructions

Compile using:

```bash
make
```

Expected compiler settings:

```bash
c++ -Wall -Wextra -Werror -std=c++98
```

Clean build files:

```bash
make clean
make fclean
```

Rebuild:

```bash
make re
```

---

# Running the Server

After compilation:

```bash
./webserv
```

Depending on the current stage of development, additional configuration files may be required.

---

# Development Notes

This repository is currently focused on:

- Building a reliable request parser
- Expanding configuration support
- Improving server/client lifecycle management
- Integrating CGI execution
- Refining event-driven architecture

As development progresses, components may be reorganized or redesigned.

---

# Future Work

Planned milestones include:

- Full HTTP/1.1 compliance
- GET support
- POST support
- DELETE support
- Virtual server support
- File uploads
- Autoindex generation
- Configurable error pages
- Persistent connections
- Improved CGI support
- Stress testing
- Benchmarking
- Additional protocol validation

---

# 42 Project Context

Webserv is one of the largest systems-programming projects in the 42 curriculum.

The project combines concepts from:

- Operating Systems
- Networking
- Process Management
- Event-Driven Programming
- Object-Oriented Design
- Resource Management

Its purpose is not only to build a functioning HTTP server, but also to understand the architecture and challenges faced by production web servers.

---

<div align="center">

Built with C++98, sockets, and many hours of debugging.

</div>
