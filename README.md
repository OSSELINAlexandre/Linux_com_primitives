# Linux_com_primitives

Utilization of Linux set's of built-in communication primitive.

## The primitives used to execute code

## Fork

**Fork**, also known as *"heavy process"* duplicates in central memory every segment of the source code.
Therefore, the forked process has its initialized and uninitialized data, code and library duplicated from the father process. The management of both processes (father and son) are independant after the fork.

**Fork** is a main Linux primitive used by the OS's: every Linux process (except the swapper process n°0) is created through this primitive.

The *init* (process n°1) will fork all declared processes from the */etc/inittab* conf file to create all processes for the OS to function.

## Replacement

**Replacement** is a set of primitive that replace the calling source code with the one provided as an argument of the primitive. It can either be a built-in command (which binary is placed in */bin*) or a personal binary. After a call to a replacement primitive, every segment of the code, that were previously aforementioned, are replaced in central memory.

**Replacement** and **fork** are two basics fonctionalities, and it is the way a command can be executed from the prompt.

First, the bash process fork, then it replaces the son's code with the one provided by the command from prompt.

## Threads

**Threads** are also known as *"light process"*. Data and code sections aren't duplicated and are the same one in central memory for all threads created by a process. However, the OS keep tracks of each thread by saving all states of register and stack for every threads.

## Communication between processes

A process has its own segments (code, data, library) that is protected from other processes: it cannot communicates with other processes.

It exists two categories of primitives in order to establish a communication between two isolated processes :

	=> Either by the file management system (with the pipes or sockets).
	=> Or by the IPC tools provided by the OS (semaphore, message queue or with shared segment).

Sockets are used for inter machine communication whereas the other tools are used for a same machine. 
	
## Pipes

## msgQueue

the message queue is an inter-process communication tool. It provides a solution to communication structures through processes.

I use this tools for a simple booking theater system. The software is separated in a basic server-client architecture. I use the message to exchange the informations related to the reservation or consultation of theater shows, and the information realted to a given show. 

Message queue's are bi-directional, identified by an id and fairly simple and fun to use.

## signals

Another way to send informations to another process is through the signals. Exception are directly computed by the process, whereas signals are executed only once the process is re-elected. Once it is re-elected, the program first execute the signals to be computed for the given process. Said it in another way, it means that if you want to interupt a process that isn't relected for a while, you will need to wait this amount of time before the process end (contrary to the exception). 

Signals needs a signal handler, and the posix implementation through signal is fairly easy to use. Once you created a handler for a peculiar signal, you can implement the behavior wanted. Once the signals are treated, the normal execution of the given process continues. 




