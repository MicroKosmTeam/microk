# MicroKosm
**The operating system from the future...at your fingertips**  

*_MicroKosm is going to EUCYS 2023!_*  

## What is MicroKosm all about?
*_Warning: This project is in active development._*  
*_It is NOT ready for use._*  
*_Do NOT use it on production hardware._*  

MicroKosm (or MicroK for short) is a new ecosystem based on the microkernel bearing the same name.
It is written from scratch, learning from the big mistakes of the past and using what we know is valid to leap forward.
It archieves a great deal great speed and a high degree of security, all while mantaining a small footprint and an extreme flexibility.
The kernel is written in C++, and is guaranteed to remain as small as possible without compromising usability and guaranteing the longest API and ABI stability possible. Module creators are free to use whichever language they prefer, providing that it can link to C functions.  

The configuration and depends greatly on the usecase.  
For example, in a simple low-powered hard real time system with only one function, it can be build for minimal resource usage. This is in a manner similar to RTOS, with the difference that future expansion is not precluded and that programs will be mostly compatible with the whole MicroKosm ecosystem.  
On the opposite end of the spectrum, it can be adapted to a magestic configuration allowing kernel instances on different machines in subnodes to be all under the supervision of a master instance. This allows supercomputer nodes to communicate with eachother as if they were simple processes, instead of relying on INK/Linux.  


For more information about the operating system, please counsult the [Introduction to MicroK](doc/INTRODUCTION_TO_MICROK.md) document in this repository's documentation. Then, you can read the documentation found in the kernel and MKMI repositories.  

## Contribute
Contributions are gladly accepted. If you think you have a good idea and some good code, please feel free to create a pull request.
If you want to become part of the project, just contact [@FilippoMutta](https://github.com/FilippoMutta).  
Any help is dearly appreciated.  
