# MicroK
**The operating system from the future...at your fingertips**

*_Warning: This project is in active development._*
*_It is NOT ready for use._*
*_Do NOT use it on production hardware._*

MicroKernel (or MicroK for short) is a hybrid-kernel written from scratch. It tries to reach both a great deal great speed and a high degree of security, all while mantaining a small footprint and an extreme flexibility.
It aims to reach these ambitious objectives by dividing the system in two main parts: the *Core* and the *Modules*, so get the most out of the hybrid-kernel model.

## Structure
### The *Core*
The *Core* is the centerpiece of the kernel, responsible for doing the most basic things: managing memory, scheduling and such. It also manages the lifecycle modules.
It should remain as small as it can be without creating problems, so to reduce the lines of code running in kernel mode and the potential risks associated with it.

### The *Modules*
The *Modules* are almost normal programs that run normally in user mode. They are prohibited to run any userspace syscalls, as they are registered with a special API call. Obviously, this also stops normal programs from running module-specific system calls.
It may seem obvious that, if a module is just a normal program, it can be loaded by anyone on the system. That would be a serious security risk, so that is not possibile. In fact, modules __cannot__ be started by the user, only by the kernel itself (or by a userspace command only for priviliged users).

## Contribute
Contributions are gladly accepted. If you think you have a good idea and some good code(that doesn't clash with the project's goals), please feel free to create a pull request.
If you want to become part of the project, just contact [@FilippoMutta](https://github.com/FilippoMutta). Any help is dearly appreciated.
