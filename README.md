# MicroKosm
**The operating system from the future...at your fingertips**  

*_MicroKosm is going to EUCYS 2023!_*  

## Description
*_Warning: This project is in active development._*  
*_It is NOT ready for use._*  
*_Do NOT use it on production hardware._*  

MicroKosm (or MicroK for short) is a morphing microkernel written from scratch. It tries to reach both a great deal great speed and a high degree of security, all while mantaining a small footprint and an extreme flexibility.  

For more information about the operating system, please counsult the [Introduction to MicroK](doc/INTRODUCTION_TO_MICROK.md) document in this repository's documentation. Then, you can read the documentation found in the kernel and MKMI repositories.  

## Setup

### Repos
On first run, it is necessary to download and update all the subrepos. You can do it like this: `git submodule update --init --remote`  
With that done, the repos are setup.  

### Compiling
**__WARNING:__** A cross-compiler is **strongly** recomended, as a system might not even boot if compiled with a normal system compiler.  

**__WARNING:__** Currently, only GNU compilers are supported. There are plans to change that.  

**__WARNING:__** Currently, only NASM is supported for x86 assembly. Beware that the assembly files are coded in such a way that the assembler cannot be simply "switched out". There are plans to change that.  

#### Overriding default variables
If you desire to override some compilation variables (trust me, you do), just modify the flags in the `Makefile.inc` file in the root of the project.  
By default the Makefile uses a compiler in the form `$(ARCH)-elf-*`, with $(ARCH) being the architecture targeted. If no such compiler is found in the `$PATH`, the build will fail.  

#### Creating a default config
It is reccomended to create a default (or custom) config for the kernel before running a compile job.  
This is a quick setup guide. For more information, look for the config utility README in the respective directory.  

To get a defconfig, use the following command: `Not yet implemented`  
To get a graphical ncurses-based utility to create a custom config: `make menuconfig`  

To get a text prompt-based utility to create a custom config: `make nconfig`  

Once the config is generated, the `autoconf.h` file is automatically transfered to the `include` directory in the kernel subrepo. If the operation fails, please copy the file manually.  

#### Compiling the kernel

First of all, enter the kernel directory. Then, use the following command: make kernel`  

Once executed, this command will produce the microk.elf file. If it isn't present, or if you have any problems, refer to the README in the kernel repo.  

#### Compiling MKMI

First of all, enter the kernel directory. Then, use the following command: `make mkmi`  

Once executed, this command will produce the libmkmi.so and libmkmi.a files. If they aren't present, or if you have any problems, refer to the README in the MKMI repo.  

#### Compiling modules
To compile a module, simply enter its directory and use either `make static` if you want to run the module without dynamic linker by loading it directly from the bootloader or `make dynamic` if you intend to use the dynamic linker (for more information, read MKMI's documentation).  

If the compilation fails because libmkmi cannot be found, make sure that a valid symlink or a valid copy of the libmkmi files is present in the module's directory.

### Running
#### On a virtual machine for quick testing
If it's the first time running MicroK you'll need to create an image in the root directory of the project. You can do it like this: `dd if=/dev/zero of=microk.img bs=512 count=1M`  
This will create an image of 512MB.  

You'll also need to download the binary release of Limine, compile it with your system compiler and put it in a `limine` directory ad the root of the project.  

Then, you can run `make buildimg` to partition the image and to copy files. Beware: this step will use loop0 to mount the image. Not only will this require root privileges, but it's important to check if loop0 is actually free.  

Any executable file found in /modules will be automatically moved in the image (but not automatically loaded by limine, to do that you need to edit the limine.cfg file).

If anything goes wrong, check if the loop device is actually free. Also, check that Limine is correctly installed and compiled. Finally, make sure that parted is installed.  

Once the image has been created, you'll be able to run it with: `make run-` and adding the architecture and platform. To run the virtual machine you need to have QEMU installed (and the OVMF firmware if running EFI variants). Check the makefile in the root of the repository to make sure that the options in QEMU are appropriate for your machine or for your testing neeeds.  

#### On real hardware/external virtual machine
While the image create by the previous step is bootable on real machines, you may wish to run MicroKosm in a different way.  

A valid MicroK kernel can support various boot protocols and bootloaders, depending on which are selected in the configuration phase. For more information, read the kernel's documentation.  

MicroK should be able to run on any machine, if it's compiled for the correct architecture and with the correct parameters and adjustments. Just make sure to have a serial port open so that you can read the early kernel output.  

## Contribute
Contributions are gladly accepted. If you think you have a good idea and some good code, please feel free to create a pull request.
If you want to become part of the project, just contact [@FilippoMutta](https://github.com/FilippoMutta).  
Any help is dearly appreciated.  
