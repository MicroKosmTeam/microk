# Introduction to MicroKosm

## Compilation

### Phase 1: Dowloading repositories
Get the main repository with `git clone` and then get all the subrepos by entering the root repository and doing `git submodule update --init`.

### Phase 2: Getting a cross-compiler
**Requirements:** wget, make, and a c compiler (preferably gcc), nasm.  
Enter the `./compiler` subrepo  
Execute the  `./compile.sh` script with an argument to create a cross compiler. Multiple cross compilers for various architectures can be created with this method.  
To create for x86_64, run: `./compile.sh x86_64-elf`. Or, for aarch64 do: `./compile.sh aarch64-elf`.  
The script will automatically check if the tarfiles latest supported version of GCC and Binutils are present, and will not download them twice, therefore this step can be executed offline after the first time.

### Phase 3: Compiling the components

#### Using the ./make.sh script
***Warning:*** those scripts do also phases 4 and 5, so for the first time do it the manual way to make sure it works.  
The two `./make.sh` and `./clear.sh` are used to respectively compile or clear all kernel components.  

#### Manually
Components must be compiled in this order (always being the in root repo):  
 - MKMI: `make -C mkmi static` to create the static library `./mkmi/libmkmi.a`.  
 - Modules: compile the kernel modules by entering  for each one `make -C (module directory) module`. This will generate the executable file in the `./modules` directory.  
 - Kernel: run the command `make -C microk-kernel kernel`. This will generate the kernel executable in `./microk-kernel/microk.elf`.  

### Phase 4: Creating a bootable kernel image with Limine
**Requirements:** tar, parted, losetup, mkfs.fat  
***Warning:*** this will use root privileges. If, by any means, the image creation were to fail, delete the `./img_mount` directory if present, fix the issue and try again.  
Get in the `./limine` subrepo and run normal `make`. This will generate files necessary for the limine bootloader.  
Then, create a disk image in the root repo of name `microk.img`, for example with `dd if=/dev/zero of=microk.img bs=512 count=93000 status=proress`  
After all of that, you can run `make buildimg` to create the image.  

### Phase 5: Running in QEMU
**Requirements:** qemu-system, OVMF (optional, needed for some targets)  
***Warning:*** OVFM is expected to be found (as it should be), in `/usr/share/OVMF`. If isn't present there, please make a symlink in that place that points to OVMF firware.  
Run one of the various commands like `make run-x64-efi` and you'll get a QEMU window. Uncheck the `Pause` button in the `Machine` menu.  
Keep in mind debugging of kernel components is allowed by typing `target remote localhost:1234` within GDB.  



