#include "main.h"
#include <iostream>
#include <mach-o/dyld.h>
#include <dlfcn.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/mman.h>
#include <mach/mach_vm.h>
#include <mach/mach_init.h>
#include <sys/stat.h>
#include <limits.h>
#include <thread>
#include <mach/vm_types.h>
#include <mach/vm_prot.h>
#include <sys/_types/_mach_port_t.h>
#include <string>
#include <vector>
#include <sstream>
#include <mach/mach_init.h>
#include <mach/mach_vm.h>
#include <mach/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/i386/kern_return.h>
#include <iostream>
#include <unistd.h>
#include "mach-o/dyld_images.h"
#include <mach/kern_return.h>
#include <sys/_types/_uintptr_t.h>
#include "mach-o/loader.h"

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

void real_entry(void)
{
    std::cout << "Main invoked" << std::endl;
}

__attribute__((always_inline)) inline unsigned int SizeOfMachO()
{
    unsigned int size = 0;
    struct mach_header_64 *header = (struct mach_header_64 *)_dyld_get_image_header(0);
    struct load_command *cmd = (struct load_command *)((uintptr_t)header + sizeof(struct mach_header_64));

    for (uint32_t i = 0; i < header->ncmds; i++)
    {
        if (cmd->cmd == LC_SEGMENT_64)
        {
            struct segment_command_64 *segment = (struct segment_command_64 *)cmd;

            struct section_64 *section = (struct section_64 *)((uintptr_t)segment + sizeof(struct segment_command_64));
            for (uint32_t j = 0; j < segment->nsects; j++)
            {
                if (strcmp(segment->segname, "__DATA_CONST") == 0 && strcmp(section->sectname, "__got") == 0)
                {
                    return section->addr + section->size;
                }

                section = (struct section_64 *)((uintptr_t)section + sizeof(struct section_64));
            }
        }
        cmd = (struct load_command *)((uintptr_t)cmd + cmd->cmdsize);
    }

    return size;
}

__attribute__((always_inline)) inline uintptr_t GetTextSize()
{
    unsigned int size = 0;
    struct mach_header_64 *header = (struct mach_header_64 *)_dyld_get_image_header(0);
    struct load_command *cmd = (struct load_command *)((uintptr_t)header + sizeof(struct mach_header_64));

    for (uint32_t i = 0; i < header->ncmds; i++)
    {
        if (cmd->cmd == LC_SEGMENT_64)
        {
            struct segment_command_64 *segment = (struct segment_command_64 *)cmd;

            struct section_64 *section = (struct section_64 *)((uintptr_t)segment + sizeof(struct segment_command_64));
            for (uint32_t j = 0; j < segment->nsects; j++)
            {

                if (strcmp(segment->segname, "__TEXT") == 0 && strcmp(section->sectname, "__text") == 0)
                {
                    return section->size;
                }

                section = (struct section_64 *)((uintptr_t)section + sizeof(struct section_64));
            }
        }
        cmd = (struct load_command *)((uintptr_t)cmd + cmd->cmdsize);
    }

    return size;
}

__attribute__((always_inline)) inline uintptr_t GetTextStart()
{
    unsigned int size = 0;
    struct mach_header_64 *header = (struct mach_header_64 *)_dyld_get_image_header(0);
    struct load_command *cmd = (struct load_command *)((uintptr_t)header + sizeof(struct mach_header_64));

    for (uint32_t i = 0; i < header->ncmds; i++)
    {
        if (cmd->cmd == LC_SEGMENT_64)
        {
            struct segment_command_64 *segment = (struct segment_command_64 *)cmd;

            struct section_64 *section = (struct section_64 *)((uintptr_t)segment + sizeof(struct segment_command_64));
            for (uint32_t j = 0; j < segment->nsects; j++)
            {
                if (strcmp(segment->segname, "__TEXT") == 0 && strcmp(section->sectname, "__text") == 0)
                {
                    return section->addr;
                }

                section = (struct section_64 *)((uintptr_t)section + sizeof(struct section_64));
            }
        }
        cmd = (struct load_command *)((uintptr_t)cmd + cmd->cmdsize);
    }

    return size;
}

void Decrypt(uintptr_t baseAddress)
{
    uintptr_t textSize = GetTextSize();
    uintptr_t textStart = GetTextStart();
    uintptr_t textStartAddr = (uintptr_t)baseAddress + (uintptr_t)textStart;

    uint8_t *buffer = (uint8_t *)mmap(NULL, textSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    uint8_t *original = (uint8_t *)mmap(NULL, textSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    // clone the memory
    mach_vm_protect(mach_task_self(), (uintptr_t)buffer, textSize, 0, VM_PROT_READ | VM_PROT_WRITE);

    // create buffers
    memcpy(buffer, (void *)textStartAddr, textSize);
    memcpy(original, (void *)textStartAddr, textSize);

    // decrypt
    for (size_t i = 0; i <= textSize; i++)
    {
        if (i >= 0xFC && i <= 0xFC + 0x714)
        {
            buffer[i] = original[i];
            continue;
        }

        buffer[i] = original[i] ^ 0x12345;
    }

    // protect the memory to be executable
    mach_vm_protect(mach_task_self(), (uintptr_t)buffer, textSize, 0, VM_PROT_READ | VM_PROT_EXECUTE);

    // protect the main memory to be writable
    kern_return_t k1 = mach_vm_protect(mach_task_self(), textStartAddr, textSize, 0, VM_PROT_READ | VM_PROT_WRITE);
    if (k1 != KERN_SUCCESS)
    {
        return;
    }

    // copy the decrypted memory back
    memcpy((void *)(uintptr_t)textStartAddr, (void *)buffer, textSize);

    // protect the main memory to be executable
    kern_return_t r2 = mach_vm_protect(mach_task_self(), textStartAddr, textSize, 0, VM_PROT_READ | VM_PROT_EXECUTE);
    if (r2 != KERN_SUCCESS)
    {
        return;
    }

    // unmap the memory
    munmap(buffer, textSize);
    munmap(original, textSize);

    return;
}

void entry_point(void)
{
    Dl_info info;
    dladdr((void *)entry_point, &info);
    uintptr_t baseAddress = (uintptr_t)info.dli_fbase;

    mach_vm_size_t size = SizeOfMachO();

    uint8_t *buffer = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    // clone the memory
    mach_vm_protect(mach_task_self(), (uintptr_t)buffer, size, 0, VM_PROT_READ | VM_PROT_WRITE);
    memcpy(buffer, (void *)baseAddress, size);
    mach_vm_protect(mach_task_self(), (uintptr_t)buffer, size, 0, VM_PROT_READ | VM_PROT_EXECUTE);

    uintptr_t decrypt = (uintptr_t)Decrypt + (uintptr_t)buffer - baseAddress;

    // call the decrypt function
    ((void (*)(uintptr_t))decrypt)(baseAddress);

    munmap(buffer, size);

    std::cout << "Did decrypt" << std::endl;

    real_entry();

    std::cout << "Did real entry" << std::endl;
}
