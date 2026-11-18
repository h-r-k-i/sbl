# SBL, or the Simple/Staged Boot Loader, Words That Are The Polar Opposite Of What This Project Does

Professionalism is down the drain right now and so for now I'm just going to pretend it's not an issue.
Mind you, this is supposed to basically be my portfolio. Then again,
[nhentai-bullshit](https://github.com/h-r-k-i/nhentai-bulllshit) is in this account so I think the
question of "will this get me a job" is closed now.

This repository exists so SBL and Neko can exist in separate directories so Neko doesn't have to larp
being a bootloader and I can decide to physically switch between working on SBL and working on Neko
instead of being tempted to work on both.

![](https://pbs.twimg.com/media/HXbEZBzXUAA3pVr.jpg?name=medium)

Let's pretend that we can ever be professional. SBL is a bootloader that does some GRUB-esque things and
then loads a kernel with user input. For a project done by a college junior with no prior experience with
systems development or operating systems it's kinda impressive. If I were a software engineer at Microsoft
writing code like this I'd probably be sent to the morgue.

The bootloader is called SBL for Staged Boot Loader or that's the name I give it because I genuinely forgot
the original name. On a good day it's SBL. On a normal one like the one I had when I "finished" adding ELF
support it's Shitty Bitchass Liar. It might've been called "Simple Boot Loader" one time ago when it was
supposed to basically be a stub attached to the larger Neko kernel but those days have long since passed.

## Prologue

I'm not doing this shit again. Claude, generate me a prologue:

> *Claude here. Fine.*
>
> In the beginning there was the BIOS, and the BIOS loaded 512 bytes to `0x7C00`, and it was not good.
> Then those 512 bytes loaded a second stage, which asked the BIOS for a memory map, the VBE modes, and
> a few other things it probably didn't need, and wrote them all down in a linked list that the author
> calls the Arc-C map. Then a third stage, written in 16-bit C++ against the explicit advice of everyone,
> learned to read FAT32, learned to parse ELF, and learned to ask you, the user, which kernel you'd like
> to boot today. Then it turned off the lights in real mode, set up a GDT and an IDT, flipped bit 0 of
> CR0, and jumped into your kernel with the map in hand.
>
> SBL started life as the boot code glued to the front of Neko. It grew up, moved out, and now lives
> here on its own. It is not GRUB. It does not want to be GRUB. GRUB is still behind you.


## So... what does it actually do?

It loads an operating system.

Since some people don't use "trust me bro" as proper documentation and demand proper information on what
the program actually does here's a section-by-section description of what each thing does:

1. Stage 1, in `boot/s1.s`. The most proper one. Loads the second stage and lets itself get torn by
   the wolves. Also enables unreal mode so I can do some hacky pointer bullshit.
2. Stage 2, in `boot/s2/`: Written in a nightmarish conglomerate of assembly and larper-mode 16-bit C,
   this part grabs a memory map of choice (whose choice? the BIOS's), pokes at the BIOS for more information
   because my inferiority complex wasn't big enough, and shoves it all haphazardly into a linked list for
   some poor soul to try to retrieve it and discover my lack of a will to live.
   Some funny bits in there too. Check it out if you survived the nightmare of seeing code that could
   only have been written by a college junior deliriously high off of diphenhydramine and nicotine.
3. Stage 3, in `boot/s3/`: 16-bit C++ now because I am an apostate and didn't wanna rewrite my code.
   It has a disk reader, an ELF loader, console output, and a shutdown function. And once the user
   actually gets the proper kernel to be loaded, it shits out a kernel in upper memory, sets up
   the GDT and IDT, enters protected mode, and jumps to the kernel. Genericized, because any
   Arc-C-compliant kernel works. Documentation on what makes an Arc-C-compliant kernel in approximately
   700 years. I have to cry first.

Anyone who complains about `boot/s3/drivers/` not being drivers is gonna be "found" dead on Peachtree.
Which one? Well, that's where the scare quotes come in.

## Getting Started

> [!NOTE]
> It's README dependency all the way down. That never changed. The difference now is some of them are
sometimes written by Claude when I can't give a shit anymore.

### Prerequisites

- Realistically speaking device-wise you only need a computer that can run QEMU but if you want to torment
  yourself like that then you need at least a 32-bit CPU and a motherboard that supports LBA indexing (most
  modern ones do). From recent tests this OS can run itself off of a SATA drive if written to disk. Some
  early versions were tested and validated through USB. I will not be testing that again because I'm scared
  of xHCI.
  I personally recommend QEMU although any emulator that supports 32-bit x86 should work.
  (if you don't use QEMU God help you)
  (use the Make on a 4GiB system I fucking ***dare*** you to).  
- Due to the nature of OSes you will need a cross-compiler to build the bootloader.
  If you don't know how to build a cross-compiler, search up "osdev xcc" and if that doesn't show anything
  "osdev cross compiler." It is not that hard; it just takes a while. Grab a coffee. And four RedBulls.
  And a pack of cigarettes.
- Speaking of compilers, you specifically need a C compiler that can compile for 16-bit x86. I use GCC here.
  You also need a C++ compiler that can do the same. I use G++ here.
  You will also need NASM and ld for linking. This isn't a hard requirement; just do `sudo apt install nasm`.
  NASM doesn't even need to be cross-compiled.
- You will need disk image stuff like `sfdisk` and `mkfs.fat` and `mmd` and `mcopy` because
  I genuinely woke up one day and decided to ruin my life by implementing FAT support.
  This is genuinely how that happened, I'm not even joking.
- You will need Make because God has abandoned us. Otherwise unchanged except now God has abandoned us even more.

> [!WARNING]
> The Makefile in its current state hardcodes locations. Unless you're *also* a Renee who compiled
> her cross-chain compiler in `~/opt/cross/` then you gotta change those. Your problem.

> [!IMPORTANT]
> This project is incomplete. Search up what that means. Also learn this fun fact: with OS dev,
> there's never a finish line. You're just deliriously searching for one in the dark. Be warned.
> GRUB's behind you. With a ski mask and a Glock 21 Gen4. Don't try to run. It has a switch.

### Building


```bash

# clone repo
git clone https://github.com/h-r-k-i/sbl.git
cd sbl

# Build the bootloader (spits out sbl.bin and a 512MiB FAT32 test image, sbl.img)
make

# Optionally drop a kernel ELF into /BOOT/ on the test image while you're at it
make KERNEL=path/to/kernel.elf
# or rename it on the way in (it's FAT, keep it 8.3)
make KERNEL=path/to/kernel.elf KERNEL_NAME=KERNEL.ELF
```

### Running
```bash

# Run the bootloader in QEMU
make run

# If it works you'll see the following text:
# 
# Please type the 8.3 directory of the kernel you want to boot,
# or enter Q to shut down the device.
# Do not enter drive letters.
# Max length is 255 characters.
# sbl.harukimedia.org# _
# 
# The cursor at the end is flashing.
# 
# If it doesn't work, don't worry. There is no God to save you this time. Just qemu.log.
```

### Debugging
```bash
# starts QEMU paused with a GDB stub
make debug

# preferably in a better terminal
gdb -ex "target remote :1234"
```

May God have mercy on your soul. I sold mine already. In my defense, Takis are really fucking good.

### Cleanup

```bash
make clean
```

Thank God this works all the time except for when it doesn't.

### Usage Example

uhhhhhh

lol

check youtube maybe

## Roadmap

- [x] Booting
- [X] Getting to protected mode without eating shit 99% of the time (it's still 98% but that's an improvement!)
      (think of it like this: successes jumped from 1% to 2%. That's 100% improvement)
- [X] FAT32 and ELF reading
- [ ] Bugfixes
- [ ] More bugfixes
- [ ] Getting that one part of the bootloader to actually load the VBE mode listed in the Arc-C map instead
      of basically going "lol figure it out yourself dumbass" and forcing the kernel to handle that itself
      (mind you 32-bit mode doesn't permit BIOS interrupts)
- [ ] Fixing the slop.


what is there to say about it? It's a bad bootloader

## References

- [OSDev Wiki](https://wiki.osdev.org) from which half this code was pilfered from.
- [The IA-32 Intel Architecture Software Developer's Manual](https://www.cs.cmu.edu/~410/doc/intel-sys.pdf)
- [15-410 Project 4](https://www.cs.cmu.edu/~410-s07/p4/p4-boot.pdf) which this has massively diverged from
- Wikipedia
- Ralf Brown's Interrupt List

## Repository layout

Who do you think I am? Claude?

> *Yes, apparently.*

```
.
├── Makefile                 # builds all three stages, glues them into sbl.bin, makes sbl.img
├── boot/
│   ├── s1.s                 # Stage 1: the 512-byte MBR. Loads stage 2, enables unreal mode
│   ├── s2/                  # Stage 2: linked at 0x1000, max 12KiB
│   │   ├── s2.s             #   entry + assembly glue
│   │   ├── s2.c             #   16-bit C: memory map (E820/E801/88h), VBE info, builds the Arc-C map
│   │   ├── s2.h             #   structs for the above
│   │   ├── s2func.s         #   BIOS call wrappers C can't do itself
│   │   └── s2.ld            #   linker script (pads to size, asserts it fits)
│   └── s3/                  # Stage 3: linked at 0x8000, max ~50KiB
│       ├── s3.s             #   entry stub
│       ├── s3.cpp / s3.hpp  #   the main event: kernel prompt, ELF load, GDT/IDT, protected mode jump
│       ├── s3.ld            #   linker script
│       ├── drivers/         #   NOT DRIVERS (see THESE_ARENT_DRIVERS_SYFM)
│       │   ├── consoleio.cpp    # text-mode console I/O
│       │   ├── disk.cpp/.s      # BIOS extended disk reads (int 13h)
│       │   ├── fatfsd.cpp       # FAT32 reader
│       │   ├── elf.cpp          # ELF loader
│       │   ├── protected.s      # protected mode entry + jump to kernel
│       │   ├── pwm.s            # shutdown + busy-wait
│       │   └── io.s             # keyboard input + cursor control (BIOS)
│       └── include/         #   headers, incl. sblheader.hpp (the Arc-C map format)
└── LICENSE
```

Plus a few files with spaces in their names (a hall of shame, some ASCII art about writing a disk driver, and
retired protected-mode assembly) that the build ignores. They're load-bearing emotionally. Leave them.

Build output lands in `build/` (objects, ELFs, linker maps), with `sbl.bin` and `sbl.img` in the repo root.
`sbl.img` is an MBR-partitioned disk with one FAT32 partition starting at sector 2048. Stage 1 goes in the
MBR and stages 2 and 3 live in the gap before the partition.

## License

This project is licensed under the [MIT License](LICENSE) but frankly why would you want to use this shit?

## Acknowledgements

I HATE x86 I HATE CR0 I HATE UNREAL MODE I HATE THE 286 I HATE QEMU nah qemu's chill everyone else can fuck themselves.

this is bad and ahh and shid and crap and dookie granted less so now but it's still bad and ahh and shid and crap and dookie.