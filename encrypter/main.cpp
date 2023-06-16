// Dylatte encrypter.

#include <iostream>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

uintptr_t GetTextSize(uint8_t *fBuffer)
{
    unsigned int size = 0;
    struct mach_header_64 *header = (struct mach_header_64 *)fBuffer;
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

uintptr_t GetTextStart(uint8_t *fBuffer)
{
    unsigned int size = 0;
    struct mach_header_64 *header = (struct mach_header_64 *)fBuffer;
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

uintptr_t mainFuncAddress = 0;
size_t mainFuncLength = 0;

uint8_t *EncryptFileContents(uint8_t *fileContents, size_t fileLength, uintptr_t encryptionKey)
{
    uintptr_t textSize = GetTextSize(fileContents);
    std::cout << "Text size 0x" << std::hex << textSize << std::endl;
    uintptr_t textStart = GetTextStart(fileContents);
    uintptr_t textEnd = textStart + textSize;
    std::cout << "Text start 0x" << std::hex << textStart << std::endl;
    std::cout << "Text end 0x" << std::hex << textEnd << std::endl;

    uint8_t *result = (uint8_t *)malloc(fileLength);
    for (size_t i = 0; i <= fileLength; i++)
    {
        // if the address is in the main function we have to avoid encrypting it
        if (i >= mainFuncAddress && i <= mainFuncAddress + mainFuncLength)
        {
            result[i] = fileContents[i];
            continue;
        }

        // if the address is within the macho header we have to avoid encrypting it
        if (i < textStart)
        {
            result[i] = fileContents[i];
            continue;
        }

        // if the address is after the text section we have to avoid encrypting it
        if (i >= textEnd)
        {
            result[i] = fileContents[i];
            continue;
        }

        result[i] = fileContents[i] ^ encryptionKey;
    }
    return result;
}

int main()
{
    std::cout << "Enter the path to the dylib: ";
    std::string dylibPath;
    std::cin >> dylibPath;

    // read the dylib file into memory, the path can be relative or absolute
    FILE *dylibFile = fopen(dylibPath.c_str(), "rb");
    if (!dylibFile)
    {
        std::cout << "Failed to open the dylib file" << std::endl;
        return 1;
    }

    // get the length of the dylib file
    fseek(dylibFile, 0, SEEK_END);
    size_t dylibFileLength = ftell(dylibFile);
    fseek(dylibFile, 0, SEEK_SET);

    // allocate a buffer to store the dylib file contents
    uint8_t *dylibFileContents = (uint8_t *)malloc(dylibFileLength);

    // read the dylib file contents into the buffer
    fread(dylibFileContents, 1, dylibFileLength, dylibFile);

    // close the dylib file
    fclose(dylibFile);

    std::cout << "Enter the main function address: ";
    std::cin >> std::hex >> mainFuncAddress;

    std::cout << "Enter the length of the main function: ";
    std::cin >> std::hex >> mainFuncLength;

    uint8_t *encryptedDylibFileContents = EncryptFileContents(dylibFileContents, dylibFileLength, 0x12345);

    // write the encrypted dylib file contents to a file
    std::cout << "Enter the path to the encrypted dylib: ";
    std::string encryptedDylibPath;
    std::cin >> encryptedDylibPath;

    FILE *encryptedDylibFile = fopen(encryptedDylibPath.c_str(), "wb");
    if (!encryptedDylibFile)
    {
        std::cout << "Failed to open the encrypted dylib file" << std::endl;
        return 1;
    }

    fwrite(encryptedDylibFileContents, 1, dylibFileLength, encryptedDylibFile);

    fclose(encryptedDylibFile);

    return 0;
}