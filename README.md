# Linux_com_primitives

Utilization of Linux set's of communication primitive built-in the OS.
## "Fork"

**Fork**, also known as *"heavy process"* duplicates in central memory every segment of the source code.
Therefore, the forked process has its initialized and uninitialized data, code and library duplicated from the father process. The management of both processes (father and son) are independant after the fork.

**Fork** is a main Linux primitive used by the OS's: every Linux process (except the swapper process n°0) is created through this primitive.

The *init* (process n°1) will fork all declared processes from the */etc/inittab* conf file to create all processes for the OS to function.

## "Replacement"

**Replacement** is a set of primitive that replace the calling source code with the one provided as an argument of the primitive. It can either be a built-in command (which binary is placed in */bin*) or a personal binary. After a call to a replacement primitive, every segment of the code, that were previously aforementioned, are replaced in central memory.

**Replacement** and **fork** are two basics fonctionalities, and it is the way a command can be executed from the prompt.

First, the bash process fork, then it replaces the son's code with the one provided by the command from prompt.

## "Threads"

**Threads** are also known as *"light process"*. Data and code sections aren't duplicated and are the same one in central memory for all threads created by a process. However, the OS keep tracks of each thread by saving all states of register and stack for every threads.

