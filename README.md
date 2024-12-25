# Linux_com_primitives

Utilization of Linux set of communication primitive built-in the OS.
## "Fork"

**Fork**, also known as *"heavy process"* duplicates in central memory every segment of the source code.
Therefore, the forked process has its initialized and uninitialized data, code and library duplicated from the father processes. The management of both processes (father and son) are independant after the fork.

**Fork** is a main Linux primitive used by the OS's in order to create new processes.

The *inittab* (process 0) will fork with all processes decalred in the initab conf file to create all necessary process for the OS to function.

## "Replacement"

**Replacement** is a set of primitive that replace the calling source code with the one provided as an argument. It can either be a command (which binary is placed in /bin) or a personal binary. After a call to a replacement primitive, every segment of the code, previously aforementioned, are replaced in central memory.

**Replacement** and **fork** are two basics fonctionalities, and it is the way a command can be executed from the prompt.

First, the bash process fork its own source code, then it replaced the son's with the provided command from prompt.

## "Threads"

**Threads** are also known as *"light process"*. Data and code section aren't duplicated and are the same one in central memory for all threads. However, the OS keep tracks of each thread by saving all states of register and stack for each threads.

