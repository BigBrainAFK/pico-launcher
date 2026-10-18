#include "common.h"
#include <string.h>
#include <strings.h>
#include "fat/File.h"
#include "SaveSlotRepository.h"

#define MAX_FILE_SIZE   (128 * 1024)

/// @brief Parses a "<slot> <rom path>" line
/// @return The rom path or nullptr when the line is not valid
static const char* parseLine(const char* line, u32& saveSlot)
{
    // Check that the structure of the string is roughly what we expect
    if (line[0] < '1' || line[0] > '0' + SAVE_SLOT_MAX || line[1] != ' ' || line[2] == 0)
    {
        return nullptr;
    }

    // Calculate int from chars
    saveSlot = line[0] - '0';
    return &line[2];
}

/// @brief Terminates the line starting at line and returns the start of the next line or nullptr if there is none.
static char* splitLine(char* line)
{
    char* next = strchr(line, '\n');
    if (next)
    {
        *next++ = 0;
    }

    u32 length = strlen(line);
    if (length != 0 && line[length - 1] == '\r')
    {
        line[length - 1] = 0;
    }

    return next;
}

bool SaveSlotRepository::ReadFile(std::unique_ptr<char[]>& data, u32& length) const
{
    length = 0;
    const auto file = std::make_unique<File>();
    FRESULT result = file->Open(_filePath, FA_READ);
    if (result == FR_NO_FILE)
    {
        data = std::make_unique<char[]>(1);
        data[0] = 0;
        return true;
    }
    if (result != FR_OK)
    {
        LOG_ERROR("Couldn't open save slot file\n");
        return false;
    }

    u32 size = file->GetSize();
    if (size > MAX_FILE_SIZE)
    {
        LOG_ERROR("Save slot file is too big\n");
        return false;
    }

    data = std::make_unique<char[]>(size + 1);
    if (!file->ReadExact(data.get(), size))
    {
        LOG_ERROR("Error while reading save slot file\n");
        return false;
    }

    data[size] = 0;
    length = size;
    return true;
}

u32 SaveSlotRepository::GetSaveSlot(const TCHAR* romPath) const
{
    std::unique_ptr<char[]> data;
    u32 length;
    if (!ReadFile(data, length))
    {
        return 0;
    }

    char* line = data.get();
    while (line)
    {
        char* next = splitLine(line);
        u32 saveSlot;
        const char* path = parseLine(line, saveSlot);
        if (path && strcasecmp(path, romPath) == 0)
        {
            return saveSlot;
        }

        line = next;
    }

    return 0;
}

void SaveSlotRepository::SetSaveSlot(const TCHAR* romPath, u32 saveSlot) const
{
    std::unique_ptr<char[]> data;
    u32 length;
    if (!ReadFile(data, length))
    {
        // Dont overwrite a file we could not read that may lose the slots of all other roms
        return;
    }

    u32 romPathLength = strlen(romPath);
    auto newData = std::make_unique<char[]>(length + romPathLength + 4);
    u32 newLength = 0;

    // Keep the lines of all other roms
    char* line = data.get();
    while (line)
    {
        char* next = splitLine(line);
        u32 lineSaveSlot;
        const char* path = parseLine(line, lineSaveSlot);
        if (path && strcasecmp(path, romPath) != 0)
        {
            u32 lineLength = strlen(line);
            memcpy(&newData[newLength], line, lineLength);
            newLength += lineLength;
            newData[newLength++] = '\n';
        }

        line = next;
    }

    if (saveSlot != 0 && saveSlot <= SAVE_SLOT_MAX)
    {
        newData[newLength++] = '0' + saveSlot;
        newData[newLength++] = ' ';
        memcpy(&newData[newLength], romPath, romPathLength);
        newLength += romPathLength;
        newData[newLength++] = '\n';
    }

    const auto file = std::make_unique<File>();
    if (file->Open(_filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open save slot file for writing\n");
        return;
    }

    u32 bytesWritten;
    if (file->Write(newData.get(), newLength, bytesWritten) != FR_OK || bytesWritten != newLength)
    {
        LOG_ERROR("Error while writing save slot file\n");
    }
}

bool SaveSlotRepository::GetSavePath(const TCHAR* romPath, u32 saveSlot, TCHAR* savePath, u32 savePathLength)
{
    u32 romPathLength = strlen(romPath);
    if (romPathLength >= savePathLength)
    {
        return false;
    }

    memcpy(savePath, romPath, romPathLength + 1);

    // Just like Pico Loader we replace everything after the last dot or append when there is none
    char* extension = strrchr(savePath, '.');
    if (!extension)
    {
        extension = &savePath[romPathLength];
    }

    u32 extensionLength = saveSlot == 0 ? 4 : 5;
    if ((u32)(extension - savePath) + extensionLength >= savePathLength)
    {
        return false;
    }

    // Unsure if its safer to just set each char 1-by-1 or do it like this
    memcpy(extension, ".sav", 4);

    if (saveSlot != 0)
    {
        extension[4] = '0' + saveSlot;
    }
    extension[extensionLength] = 0;
    return true;
}
