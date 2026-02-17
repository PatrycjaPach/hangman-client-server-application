# 🎮 Network Programming Project — Client–Server Multiplayer Game

Project implements a distributed **client–server application** that allows multiple users to connect to a server and participate in a letter-guessing game (Hangman-like gameplay).

---

## 📌 Project Overview

The system consists of two applications:

* **Server** – manages clients, user accounts and game logic
* **Client** – allows a user to discover the server, log in and play the game

The server is **concurrent** and supports multiple connected clients simultaneously.

Clients do not manually enter the server IP address — the server is automatically discovered in the local network using **UDP multicast**.

---

## 🧱 Technologies & Concepts Used

* TCP sockets
* UDP sockets
* Multicast communication
* Concurrent server (multi-threaded / multi-process)
* TLV (Type-Length-Value) protocol
* Client account management
* Distributed game logic

---

## 🔎 Server Discovery — Multicast


---

## 👤 User Accounts

The system supports user accounts handled by the server.

Features:

* logging into an existing account
* searching users **by username**
* distinguishing between online and offline users

The server stores:

* username
* connection status
* game session data

---

## 🎯 Game Logic

The game is a letter-guessing game similar to Hangman.

### Server responsibilities:

* chooses a secret word
* validates guessed letters
* tracks remaining attempts
* updates the game state
* sends updates to clients

### Client responsibilities:

* sends guessed letters
* displays the current game state

The client displays:

* correctly guessed letters
  example: `_ A _ A _`
* incorrect letters
  example: `X, Q, Z`
* number of remaining attempts

---

## ⚙️ Concurrent Server

The server supports multiple clients at the same time.

Each connected client is handled independently using concurrency (e.g., threads).
The server:

* accepts multiple TCP connections
* manages separate sessions
* ensures the game state is correctly synchronized

---

## 📦 Communication Protocol — TLV

Communication between the client and server is implemented using the **TLV (Type-Length-Value)** protocol.


Advantages:

* clear parsing of messages
* extensibility
* support for multiple message types

---

## 🔄 Connection Flow

1. Client discovers server via multicast
2. Client connects via TCP
3. User logs in
4. User can search other users
5. Game session starts
6. Client sends letter guesses
7. Server returns updated game state

---

## ▶️ Running the Application

### Start the server

server compilation:
gcc -o server serv.c discovery_server.c new_clients.c handle_client.c tlv.c game.c

activation:
./server

### Start the client

client compilation:
gcc -o klient cli.c discovery.c tlv.c

activation:
./klient

### Note

Each virtual machine or computer to communicate must be on the same network


---



## 👩‍💻 Authors & Responsibilities

### Antonia Zdziebko responsibilities
• concurrent server handling using poll()
• UDP multicast server discovery
• new client handling – session initialization, flags and login assignment (new_clients.c/.h)
• client handling logic – login validation and connection management (handle_client.c/.h)
• core gameplay logic – letter guessing and word handling
• TLV-based binary communication (tlv.c/.h)


### Patrycja Pach responsibilities
* Part of the concurrent server an TCP
* Player accounts - calculating the score, saving to files, finding the best result, security
* writing down incorrect letters during the game

---

## 📝 Notes

The project demonstrates:

* practical socket programming
* LAN service discovery
* concurrent server architecture
* custom application protocol design

The application was created for educational purposes.
