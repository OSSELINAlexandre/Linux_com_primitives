# Linux communication primitives

Small C programs that show how Linux processes actually talk to each other.
Two versions of the same ideas live side by side:

| | V1 | V2 |
|---|---|---|
| Goal | The smallest program that still shows the primitive | The same primitive, with the details that bite in real use |
| Where | `v1/` | `v2/` |
| Sockets | One client, loopback `127.0.0.1:7777` | Host, port and bind address from argv or the environment. Usable between a laptop and a Raspberry Pi on the same LAN |
| Errors | `perror` on the calls that fail in the demo | `perror` / `strerror`, clean shutdown on SIGINT and SIGTERM |
| Length | One screen, one idea | Still readable. No framework |

Both trees are ISO C11 and POSIX. `gcc` on x86_64 and `gcc` on Raspberry Pi OS (aarch64) build the same sources. No extra libraries.

The folders that used to sit at the repository root (`fork/`, `replacement/`, `thread/`, `pipe/`, `signal/`, `socket/`, `msgQueue/`) were the first draft of these demos. The behaviour they were teaching is what V1 and V2 do now, including the bugs that draft had (a socket buffer written one past the end, `pthread_create` compared to `-1`, `wait()` used with no declaration).

## Build

```bash
sudo apt install build-essential   # once, on Debian / Raspberry Pi OS / Ubuntu
make            # everything, into build/
make v1         # only the small demos
make v2         # only the richer demos
make check      # build and run every demo once
make clean
```

`make help` prints the same list. To compile a single file by hand, the flags the Makefile uses are required. `-std=c11` on its own hides the POSIX functions in the headers:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L \
    -o build/v1/fork/fork_demo v1/fork/fork_demo.c
```

Thread demos also need `-pthread`.

A cross compiler is the same invocation (`make CC=aarch64-linux-gnu-gcc`). Run those binaries on the Pi, not on the machine that built them. Building natively on the Pi is the path used below.

## V1 — run each demo

Binaries land under `build/v1/…`. Run them from the repository root.

### Fork — `v1/fork`

`fork` returns twice. `-1` means no child (`errno` is set). `0` means “I am the child”. A positive value means “I am the parent, and this is the child’s pid”. The child has a copy of the parent’s memory. The parent `waitpid`s so the child does not stay a zombie.

```bash
./build/v1/fork/fork_demo
```

### Replacement (`exec`) — `v1/replacement`

A shell runs a command by forking, then the **child** calls `exec`. `exec` replaces the process image and does not return when it works. The pid stays the same. `exec_demo` loads `worker`, which exits `42`, and the parent reads that status back.

```bash
./build/v1/replacement/exec_demo
```

`worker` has to sit next to `exec_demo`. `make` puts both in `build/v1/replacement/`.

### Threads — `v1/threads`

One thread, same pid, same `shared` variable. `pthread_create` returns `0` or an error number. It does not return `-1` and it does not set `errno`.

```bash
./build/v1/threads/thread_demo
```

### Unnamed pipe — `v1/pipe/unnamed`

`pipe` + `fork`. The child reads what the parent writes. Both ends are inherited, so each side closes the end it does not use. The parent’s final `close` of the write end is what makes the child’s `read` return 0. Leave that descriptor open and the child waits forever.

```bash
./build/v1/pipe/unnamed/pipe_demo
```

### Named pipe (FIFO) — `v1/pipe/named`

A FIFO is a pipe with a path, so the two processes do not have to be parent and child. `open` blocks until the other direction is open. That wait is the rendezvous, not a deadlock. Start the two programs in either order, in two terminals:

```bash
./build/v1/pipe/named/fifo_reader            # default path /tmp/linux_com_v1.fifo
./build/v1/pipe/named/fifo_writer
./build/v1/pipe/named/fifo_writer /tmp/linux_com_v1.fifo "bonjour"
```

`mkfifo`’s mode is filtered by `umask`. `0666` with the usual umask `022` becomes `0644`.

### Signals — `v1/signals`

The child sends `SIGUSR1` to the parent. The parent blocks that signal **before** `fork` and takes it with `sigwait`, so the signal cannot be lost and cannot take the default action (kill the process). `pause()` is not used: a signal that arrives between the flag check and `pause()` is missed, and `pause` then sleeps forever.

```bash
./build/v1/signals/signal_demo
```

A signal is delivered the next time the process is scheduled. It is not a synchronous CPU exception. If the process is not running, nothing runs the handler until the kernel elects it again.

### TCP sockets — `v1/sockets`

One server, one client, then both exit. Loopback only.

```bash
./build/v1/sockets/tcp_server     # terminal 1, 127.0.0.1:7777
./build/v1/sockets/tcp_client     # terminal 2
```

`htons` puts the port in network byte order. x86_64 and the Pi are little-endian; without `htons` the peer listens on a different port. `inet_pton` already stores the address in network order, so that value is not passed through `htonl`.

`recv` does not write a `'\0'`. The first draft of this repo did `Buffer[1024] = '/0'`, which is one past the end of a 1024-byte array and is not a NUL byte. The index that gets `'\0'` is the number of bytes `recv` returned, and the read size is `sizeof buf - 1`.

V1 reads the socket once. A short greeting fits in one `recv`. TCP does not promise that in general; V2 loops until a newline.

### Message queue — `v1/msg_queue`

One System V queue, created with `IPC_PRIVATE`, used by a child after `fork`. `IPC_PRIVATE` means “make a new queue”, not “only this process may use it”. The id is an integer the child inherits. The queue survives `exit` until `msgctl(IPC_RMID)` (or a reboot). `mtype` must be greater than 0.

```bash
./build/v1/msg_queue/mq_demo
ipcs -q          # if a queue is ever left behind
ipcrm -q <id>
```

## V2 — same primitives, more of the sharp edges

```bash
make v2
```

| Demo | What it adds | Run |
|---|---|---|
| `v2/fork` | Independent memory, shared file offset after `fork`, `waitpid` status word | `./build/v2/fork/fork_demo` |
| `v2/replacement` | `execve` with an explicit environment. `DEMO` is set only because the parent put it in `envp`. A failed exec exits 127 | `./build/v2/replacement/exec_demo` |
| `v2/threads` | Two threads increment a counter. Without a mutex updates are lost. `volatile` does not fix that. With a mutex the count matches | `./build/v2/threads/thread_demo` |
| `v2/pipe/unnamed` | Two pipes, newline-framed request and reply. `./pipe_demo 20 22` prints `sum=42` | `./build/v2/pipe/unnamed/pipe_demo 20 22` |
| `v2/pipe/named` | A small multiply server. One client at a time. `quit` stops it. Write the request before opening the reply FIFO, or both sides block | see below |
| `v2/signals` | `sigaction`, a `sig_atomic_t` flag (no `printf` in the handler), `sigsuspend` so the wakeup cannot be lost, `SIGCHLD` + `waitpid` so there is no zombie | `./build/v2/signals/signal_demo` |
| `v2/sockets` | LAN TCP. `SO_REUSEADDR`, accept loop, SIGINT/SIGTERM shutdown | see below |
| `v2/msg_queue` | `ftok` key file, request type `1`, reply type = client pid, so two clients do not steal each other’s answer. `quit` removes the queue | see below |

### Named pipe

Terminal 1, then terminal 2:

```bash
./build/v2/pipe/named/server                 # FIFOs /tmp/linux_com_v2.req and .rep
./build/v2/pipe/named/client /tmp/linux_com_v2 6 7
./build/v2/pipe/named/client /tmp/linux_com_v2 quit
```

The default prefix is `/tmp/linux_com_v2`. Pass another prefix as the first argument of both programs if two people share the machine. The server unlinks its FIFOs on the way out. SIGINT and SIGTERM do the same. `SIGPIPE` is ignored, so a client that disappears becomes `EPIPE` on `write` instead of killing the server.

The client writes its line on the request FIFO and only then opens the reply FIFO. The server is blocked in `read` until that line arrives, and it opens the reply FIFO only after the read. Opening the reply side first leaves both programs stuck in `open` / `read`.

### Message queue

The typed-message idea comes from the old theater-booking draft (one queue, several `mtype`s). That draft also had several threads calling `msgrcv` on the same queue. This server is one loop.

```bash
./build/v2/msg_queue/server                  # creates /tmp/linux_com_v2_mq.key
./build/v2/msg_queue/client /tmp/linux_com_v2_mq.key "two seats"
./build/v2/msg_queue/client /tmp/linux_com_v2_mq.key quit
```

`ftok` hashes the key file’s inode. Delete that file and create it again, and the key changes: the client and the server no longer meet. The mode of the queue is `0666` with `umask` cleared for the `msgget`, so another user on the same machine can send. `ipcs -q` / `ipcrm -q` are the escape hatch if the server is killed with `SIGKILL` (which cannot run the cleanup).

### TCP between a laptop and a Raspberry Pi

IPv4 TCP. Both machines on the same LAN. Build **on each machine** (native `gcc`):

```bash
sudo apt install build-essential
make v2
```

On the Pi, take its address:

```bash
ip -4 addr
```

Start the server on the Pi. `0.0.0.0` means every IPv4 interface (Ethernet and Wi-Fi). `127.0.0.1` would mean “only programs on the Pi”, and the laptop would never connect.

```bash
./build/v2/sockets/tcp_server 0.0.0.0 7777
```

The same thing from the environment, which argv overrides when you pass arguments:

```bash
BIND_ADDR=0.0.0.0 PORT=7777 ./build/v2/sockets/tcp_server
```

On the laptop, point the client at the Pi’s address, not at `127.0.0.1`:

```bash
./build/v2/sockets/tcp_client 192.168.1.42 7777 "hello pi"
```

```bash
HOST=192.168.1.42 PORT=7777 ./build/v2/sockets/tcp_client
```

The server answers `echo: hello pi` and keeps accepting. Ctrl+C (SIGINT) or SIGTERM closes the listening socket and leaves. `SO_REUSEADDR` is set before `bind`, so starting the server again while the previous connection is in `TIME_WAIT` does not fail with `EADDRINUSE`.

The client takes a dotted IPv4 address (`inet_pton` + `htons`, so the byte-order call stays visible in the source). A hostname would need `getaddrinfo`, which hides that call.

If the client prints `Connection refused`, nothing is listening on that ip and port. If it hangs or the kernel says `No route to host`, the two machines are not on a path that carries the packet, or a firewall dropped it. On the Pi, if `ufw` is enabled:

```bash
sudo ufw allow 7777/tcp
```

Check from the laptop with the Pi’s address, for example `192.168.1.42`:

```bash
ping -c 1 192.168.1.42
```

`accept` is a loop. One `recv` is not one message: the server reads until `'\n'`, and the client sends until `send` has taken every byte. That is the difference with the V1 socket demo.

## What the primitives are

**Fork.** `fork` duplicates the process. The child gets its own copy of the code, the data and the stack (the kernel does this copy-on-write). After the call, parent and child are scheduled independently. Every process on a Linux system except pid 0 is created this way. pid 1 (`init`, or systemd) is the ancestor of the processes that bring the machine up.

**Replacement.** The `exec` family (`execv`, `execve`, `execvp`, …) throws away the caller’s code and data and loads another program in the same process. A prompt works by combining the two: the shell forks, the child execs the command, the parent waits. Replacing the parent as well would leave nobody to call `waitpid`.

**Threads.** Threads share the process’s code and data. The kernel keeps a separate set of registers and a separate stack for each one. Same pid, same variables, which is why an unprotected `counter++` loses updates. A mutex makes that increment one critical section. `volatile` only stops the compiler from caching the variable in a register.

**Pipes and FIFOs.** A process’s address space is not visible to another process. A pipe is a kernel byte stream that crosses that boundary. An unnamed pipe has no path; it is passed by inheriting the descriptors across `fork`. A FIFO (`mkfifo`) has a path, so unrelated processes can open it. Both are one direction, and both are streams: `read` can return fewer bytes than `write` sent. Closing the last write end is the end-of-file.

**Message queues.** System V queues are another kernel object, addressed by a key (`ftok` or `IPC_PRIVATE`) and used with `msgsnd` / `msgrcv`. Each message carries a type. That type is how one queue carries several conversations. The queue does not go away when the process does.

**Signals.** A signal is a notification, not a payload of any size. The default action of many signals is to kill the process. A handler installed with `sigaction` runs when the process is next scheduled, and it may only call async-signal-safe functions. The usual shape is a `volatile sig_atomic_t` flag, with the real work done back in the normal flow. `signal()` is the older call; on some systems it resets the action to the default on delivery.

**Sockets.** The calls above stay on one machine. A TCP socket is the same byte-stream idea across a network. `bind` chooses the local address, `listen` / `accept` take connections, `connect` is the client side. Numbers wider than one byte travel in network order (`htons` / `ntohs`).

## Layout

```
v1/fork                v2/fork
v1/replacement         v2/replacement
v1/threads             v2/threads
v1/pipe/unnamed        v2/pipe/unnamed
v1/pipe/named          v2/pipe/named
v1/signals             v2/signals
v1/sockets             v2/sockets
v1/msg_queue           v2/msg_queue
```

Each `.c` file says, at the top, which call is the point of the program and which return value is easy to misread. `scripts/smoke.sh` is what `make check` runs.
