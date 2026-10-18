#pragma once
#include <memory>
#include "fat/ff.h"

/// @brief The highest save slot. Slot 0 uses the regular .sav file
///        After that it goes .sav1, .sav2, ..., .sav9
///        This means the actual maximum amount of slots is N+1
#define SAVE_SLOT_MAX   9

/// @brief This keeps track of the selected save slot per rom
///        If a slot other than 0 is selected a single line of "<slot> <rom path>" is made
///        Only one line per rom
class SaveSlotRepository
{
public:
    explicit SaveSlotRepository(const TCHAR* filePath)
        : _filePath(filePath) { }

    /// @brief Gets the selected save slot for the rom at romPath
    /// @param romPath The full path of the rom
    /// @return The selected save slot or 0 when no slot was selected
    u32 GetSaveSlot(const TCHAR* romPath) const;

    /// @brief Sets the selected save slot for the rom at romPath
    /// @param romPath The full path of the rom
    /// @param saveSlot The save slot to select
    void SetSaveSlot(const TCHAR* romPath, u32 saveSlot) const;

    /// @brief Builds the save file path for the given romPath + saveSlot combination
    ///        This uses the same naming as Pico Loader
    ///        Slot 0 matches the save Pico Loader picks by default
    /// @param romPath The path or file name of the rom
    /// @param saveSlot The save slot
    /// @param savePath The buffer to write the save path to
    /// @param savePathLength The length of the savePath buffer
    /// @return true when the save path was built and false when it did not fit in the buffer
    static bool GetSavePath(const TCHAR* romPath, u32 saveSlot, TCHAR* savePath, u32 savePathLength);

private:
    const TCHAR* _filePath;

    bool ReadFile(std::unique_ptr<char[]>& data, u32& length) const;
};
