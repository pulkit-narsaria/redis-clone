# 🧠 Redis Clone in C++

A simplified Redis clone built from scratch in C++ to understand how Redis works under the hood — from TCP connections to command parsing and in-memory storage.

---

## 🚀 Project Motivation

Building Redis clone from scratch, I will learn to establish TCP server and client in C++, design my own request-response protocol, and builtd an in-memory key-value store. This will help me learn how sockets work, how data travels over the wire, how clients and servers interact.

This project isn't just about cloning Redis. It is about bringing abstract textbook knowledge to life. It made me realize how powerful low-level programming can be, and how even the simplest commands like `SET` and `GET` are backed by elegant, efficient systems.

---

## ✅ Features Implemented Till Now :

- [x] TCP Server-Client communication

---

## 🛠️ Next Planned Features

- [ ] Request-Response Protocol implementation

---

## 🔧 Getting Started

### 📦 Prerequisites

- C++17 or higher
- g++ / clang++ / MSVC
- Linux/macOS/WSL (for now)

### ⚙️ Build Instructions

```bash
# Compile the server
g++ server.cpp -o server

# Compile the client
g++ client.cpp -o client

# Run the server
./server

# In another terminal, run the client
./client

📖 Reference
This project is heavily inspired by the Build Your Own Redis guide.
Link : https://build-your-own.org/redis/
