#pragma once
#include "core/task/TaskQueue.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/IRomBrowserController.h"

/// @brief View model for the nds game details bottom sheet
///        This way its extendable in the future with more functionality
class NdsGameDetailsViewModel
{
public:
    NdsGameDetailsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController);

    ~NdsGameDetailsViewModel()
    {
        _loadTask.CancelTask();
    }

    /// @brief Gets the rom file this sheet is about
    /// @return The rom file info
    const FileInfo& GetRomFileInfo() const { return _romFileInfo; }

    /// @brief Returns whether the save slot information has been loaded
    /// @return True when the save slot information has been loaded and false otherwise
    bool IsLoaded() const { return _isLoaded; }

    /// @brief Gets the selected save slot
    /// @return The selected save slot
    u32 GetSaveSlot() const { return _saveSlot; }

    /// @brief Returns whether the save file of the selected save slot already exists
    /// @return True when the save file exists and false when it will be created on launch
    bool SaveFileExists() const { return _existingSaveSlots & (1 << _saveSlot); }

    /// @brief Gets the file name of the save file of the selected save slot
    /// @param fileName The buffer to write the file name to
    /// @param fileNameLength The length of the fileName buffer
    /// @return True when the file name was written and false when it did not fit
    bool GetSaveFileName(TCHAR* fileName, u32 fileNameLength) const;

    /// @brief Selects the next save slot with wrap-around to slot 0
    void NextSaveSlot();

    /// @brief Selects the previous save slot with wrap-around to last slot
    void PreviousSaveSlot();

    /// @brief Opens the cheats panel
    void ShowCheats();

    /// @brief Closes the game details panel
    void Close();

private:
    FileInfo _romFileInfo;
    IRomBrowserController* _romBrowserController;
    QueueTask<void> _loadTask;
    TCHAR _romPath[256];
    volatile bool _isLoaded = false;
    u32 _saveSlot = 0;
    u32 _storedSaveSlot = 0;
    u16 _existingSaveSlots = 0;

    void StoreSaveSlot();
};
