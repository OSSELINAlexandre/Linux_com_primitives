#!/usr/bin/env bash
# Run every demo once. Servers are started in the background and stopped.
# A hang hits the per-command timeout, or the caller's timeout around this script.
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
b="$root/build"
tmp=$(mktemp -d)
pids=()

stop_all() {
    local pid
    for pid in ${pids[@]+"${pids[@]}"}; do
        kill -INT "$pid" 2>/dev/null || true
    done
    sleep 0.3
    for pid in ${pids[@]+"${pids[@]}"}; do
        if kill -0 "$pid" 2>/dev/null; then
            kill -KILL "$pid" 2>/dev/null || true
        fi
    done
    wait 2>/dev/null || true
    pids=()
}

cleanup() {
    stop_all
    rm -rf "$tmp"
}
trap cleanup EXIT

say() { printf '\n== %s\n' "$*"; }

# Retry a command that races a server's startup. Connection refused is expected
# for a few dozen milliseconds; a real bug still fails after the attempts.
retry() {
    local i
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
        if "$@"; then
            return 0
        fi
        sleep 0.1
    done
    echo "gave up: $*" >&2
    return 1
}

wait_for() {
    local path=$1
    local i
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
        if [[ -e "$path" ]]; then
            return 0
        fi
        sleep 0.1
    done
    echo "missing $path" >&2
    return 1
}

# wait_exit <pid> — give a background demo a few seconds to finish on its own.
wait_exit() {
    local pid=$1
    local i
    for i in $(seq 1 50); do
        if ! kill -0 "$pid" 2>/dev/null; then
            wait "$pid" || true
            return 0
        fi
        sleep 0.1
    done
    echo "pid $pid did not exit" >&2
    return 1
}

say "v1 fork"
timeout 5 "$b/v1/fork/fork_demo" >"$tmp/v1-fork.txt"
grep -q "child: pid=" "$tmp/v1-fork.txt"
grep -q "value=1 (still the parent's copy)" "$tmp/v1-fork.txt"

say "v1 replacement"
timeout 5 "$b/v1/replacement/exec_demo" >"$tmp/v1-exec.txt"
grep -q "worker: replaced image" "$tmp/v1-exec.txt"
grep -q "worker exit status=42" "$tmp/v1-exec.txt"

say "v1 threads"
timeout 5 "$b/v1/threads/thread_demo" >"$tmp/v1-th.txt"
grep -q "after join shared=42" "$tmp/v1-th.txt"

say "v1 unnamed pipe"
timeout 5 "$b/v1/pipe/unnamed/pipe_demo" >"$tmp/v1-pipe.txt"
grep -q "received 'hello from parent'" "$tmp/v1-pipe.txt"

say "v1 named pipe"
timeout 5 "$b/v1/pipe/named/fifo_reader" "$tmp/v1.fifo" >"$tmp/v1-fifo-r.txt" &
pids+=($!)
wait_for "$tmp/v1.fifo"
timeout 5 "$b/v1/pipe/named/fifo_writer" "$tmp/v1.fifo" "ping-fifo" >"$tmp/v1-fifo-w.txt"
wait_exit "${pids[0]}"
pids=()
grep -q "got 'ping-fifo'" "$tmp/v1-fifo-r.txt"

say "v1 signals"
timeout 5 "$b/v1/signals/signal_demo" >"$tmp/v1-sig.txt"
grep -q "sigwait delivered" "$tmp/v1-sig.txt"

say "v1 message queue"
timeout 5 "$b/v1/msg_queue/mq_demo" >"$tmp/v1-mq.txt"
grep -q "text='hello from child'" "$tmp/v1-mq.txt"

say "v1 sockets"
"$b/v1/sockets/tcp_server" >"$tmp/v1-srv.txt" &
pids+=($!)
retry timeout 2 "$b/v1/sockets/tcp_client" >"$tmp/v1-cli.txt"
wait_exit "${pids[0]}"
pids=()
grep -q "got 'hello from client'" "$tmp/v1-srv.txt"
grep -q "got 'hello from server'" "$tmp/v1-cli.txt"

say "v2 fork"
timeout 5 "$b/v2/fork/fork_demo" >"$tmp/v2-fork.txt"
grep -q "section memory: parent value=1 child value=100" "$tmp/v2-fork.txt"
grep -q "section fd: file='before" "$tmp/v2-fork.txt"
grep -q "section wait: exit status=7" "$tmp/v2-fork.txt"

say "v2 replacement"
timeout 5 "$b/v2/replacement/exec_demo" >"$tmp/v2-exec.txt"
grep -q "DEMO=raspberry" "$tmp/v2-exec.txt"
grep -q "worker exit status=42" "$tmp/v2-exec.txt"

say "v2 threads"
timeout 15 "$b/v2/threads/thread_demo" >"$tmp/v2-th.txt"
grep -q "safe counter=400000 expected=400000" "$tmp/v2-th.txt"

say "v2 unnamed pipe"
timeout 5 "$b/v2/pipe/unnamed/pipe_demo" 20 22 >"$tmp/v2-pipe.txt"
grep -q "sum=42" "$tmp/v2-pipe.txt"

say "v2 named pipe"
"$b/v2/pipe/named/server" "$tmp/fifo2" >"$tmp/v2-fifo-s.txt" &
pids+=($!)
wait_for "$tmp/fifo2.req"
retry timeout 3 "$b/v2/pipe/named/client" "$tmp/fifo2" 6 7 >"$tmp/v2-fifo-c.txt"
grep -q "product=42" "$tmp/v2-fifo-c.txt"
timeout 5 "$b/v2/pipe/named/client" "$tmp/fifo2" quit >"$tmp/v2-fifo-q.txt"
wait_exit "${pids[0]}"
pids=()
grep -q "server: stopped" "$tmp/v2-fifo-s.txt"

say "v2 signals"
timeout 5 "$b/v2/signals/signal_demo" >"$tmp/v2-sig.txt"
grep -q "alarm and child reaped, status=0" "$tmp/v2-sig.txt"

say "v2 sockets loopback"
"$b/v2/sockets/tcp_server" 127.0.0.1 17778 >"$tmp/v2-sock-s.txt" &
pids+=($!)
retry timeout 3 "$b/v2/sockets/tcp_client" 127.0.0.1 17778 "ping-loopback" >"$tmp/v2-sock-c.txt"
grep -q "reply='echo: ping-loopback'" "$tmp/v2-sock-c.txt"
stop_all
grep -q "server: stopped" "$tmp/v2-sock-s.txt"

say "v2 sockets every interface"
"$b/v2/sockets/tcp_server" 0.0.0.0 17779 >"$tmp/v2-sock-s2.txt" &
pids+=($!)
retry timeout 3 "$b/v2/sockets/tcp_client" 127.0.0.1 17779 "ping-any" >"$tmp/v2-sock-c2.txt"
grep -q "reply='echo: ping-any'" "$tmp/v2-sock-c2.txt"
grep -q "listening on 0.0.0.0:17779" "$tmp/v2-sock-s2.txt"
stop_all

say "v2 sockets via environment"
BIND_ADDR=127.0.0.1 PORT=17780 "$b/v2/sockets/tcp_server" >"$tmp/v2-sock-s3.txt" &
pids+=($!)
retry timeout 3 env HOST=127.0.0.1 PORT=17780 "$b/v2/sockets/tcp_client" >"$tmp/v2-sock-c3.txt"
grep -q "reply='echo: hello from client'" "$tmp/v2-sock-c3.txt"
stop_all

say "v2 message queue"
"$b/v2/msg_queue/server" "$tmp/mq.key" >"$tmp/v2-mq-s.txt" &
pids+=($!)
wait_for "$tmp/mq.key"
retry timeout 3 "$b/v2/msg_queue/client" "$tmp/mq.key" "hello-mq" >"$tmp/v2-mq-c.txt"
grep -q "reply='ack: hello-mq'" "$tmp/v2-mq-c.txt"
timeout 5 "$b/v2/msg_queue/client" "$tmp/mq.key" quit >"$tmp/v2-mq-q.txt"
wait_exit "${pids[0]}"
pids=()
grep -q "reply='bye'" "$tmp/v2-mq-q.txt"
grep -q "queue removed" "$tmp/v2-mq-s.txt"

echo
echo "all demos ok"
