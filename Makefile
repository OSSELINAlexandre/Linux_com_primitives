# Build every demo into build/. Nothing is installed on the system.
#
#   make            # v1 and v2
#   make v1         # smallest demos
#   make v2         # richer demos, including the LAN sockets
#   make check      # build, then run each demo once
#   make clean
#
# CC is the compiler. The same sources build on x86_64 and on a Raspberry Pi
# (aarch64). A cross compiler works the same way, but the binaries it
# produces will not run on the machine that built them:
#
#   make CC=aarch64-linux-gnu-gcc
#
# -std=c11 alone hides POSIX declarations in glibc. _POSIX_C_SOURCE puts
# sigaction, sigwait, pthread extras and the socket API back in the headers.
# Thread programs also need -pthread (set on those targets below).

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
BUILD   := build

.DELETE_ON_ERROR:

.PHONY: all v1 v2 clean check help

V1_BINS := \
	$(BUILD)/v1/fork/fork_demo \
	$(BUILD)/v1/replacement/exec_demo \
	$(BUILD)/v1/replacement/worker \
	$(BUILD)/v1/threads/thread_demo \
	$(BUILD)/v1/pipe/unnamed/pipe_demo \
	$(BUILD)/v1/pipe/named/fifo_reader \
	$(BUILD)/v1/pipe/named/fifo_writer \
	$(BUILD)/v1/signals/signal_demo \
	$(BUILD)/v1/sockets/tcp_server \
	$(BUILD)/v1/sockets/tcp_client \
	$(BUILD)/v1/msg_queue/mq_demo

V2_BINS := \
	$(BUILD)/v2/fork/fork_demo \
	$(BUILD)/v2/replacement/exec_demo \
	$(BUILD)/v2/replacement/worker \
	$(BUILD)/v2/threads/thread_demo \
	$(BUILD)/v2/pipe/unnamed/pipe_demo \
	$(BUILD)/v2/pipe/named/server \
	$(BUILD)/v2/pipe/named/client \
	$(BUILD)/v2/signals/signal_demo \
	$(BUILD)/v2/sockets/tcp_server \
	$(BUILD)/v2/sockets/tcp_client \
	$(BUILD)/v2/msg_queue/server \
	$(BUILD)/v2/msg_queue/client

all: v1 v2

v1: $(V1_BINS)

v2: $(V2_BINS)

# build/v1/fork/fork_demo comes from v1/fork/fork_demo.c
$(BUILD)/%: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/v1/threads/thread_demo: CFLAGS += -pthread
$(BUILD)/v2/threads/thread_demo: CFLAGS += -pthread

check: all
	bash scripts/smoke.sh

clean:
	rm -rf $(BUILD)

help:
	@echo "make            build v1 and v2 into $(BUILD)/"
	@echo "make v1         smallest demo of each primitive"
	@echo "make v2         richer demos (LAN TCP sockets, error handling)"
	@echo "make check      build and run every demo once"
	@echo "make clean      delete $(BUILD)/"
	@echo "make CC=...     use another gcc (native on the Pi, or a cross compiler)"
