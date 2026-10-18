#include "common.h"
#include <memory>
#include <string.h>
#include "saves/SaveSlotRepository.h"
#include "NdsGameDetailsViewModel.h"

NdsGameDetailsViewModel::NdsGameDetailsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController)
    : _romFileInfo(romFileInfo), _romBrowserController(romBrowserController)
{
    _romPath[0] = 0;
    _loadTask = _romBrowserController->GetIoTaskQueue()->Enqueue([this] (const vu8& cancelRequested)
    {
        _romBrowserController->GetFullFilePath(_romFileInfo, _romPath, sizeof(_romPath) / sizeof(_romPath[0]));
        u32 saveSlot = _romBrowserController->GetSaveSlotRepository().GetSaveSlot(_romPath);

        // Check which slots already have a save file
        // Then we can also show if a save is new or not
        const u32 savePathLength = sizeof(_romPath) / sizeof(_romPath[0]);
        auto savePath = std::make_unique<TCHAR[]>(savePathLength);
        u16 existingSaveSlots = 0;
        for (u32 slot = 0; slot <= SAVE_SLOT_MAX; slot++)
        {
            if (SaveSlotRepository::GetSavePath(_romPath, slot, savePath.get(), savePathLength) &&
                f_stat(savePath.get(), nullptr) == FR_OK)
            {
                existingSaveSlots |= 1 << slot;
            }
        }

        _saveSlot = saveSlot;
        _storedSaveSlot = saveSlot;
        _existingSaveSlots = existingSaveSlots;
        _isLoaded = true;
        return TaskResult<void>::Completed();
    });
}

bool NdsGameDetailsViewModel::GetSaveFileName(TCHAR* fileName, u32 fileNameLength) const
{
    return SaveSlotRepository::GetSavePath(_romFileInfo.GetFileName(), _saveSlot, fileName, fileNameLength);
}

void NdsGameDetailsViewModel::NextSaveSlot()
{
    if (_isLoaded)
    {
        _saveSlot = _saveSlot == SAVE_SLOT_MAX ? 0 : _saveSlot + 1;
    }
}

void NdsGameDetailsViewModel::PreviousSaveSlot()
{
    if (_isLoaded)
    {
        _saveSlot = _saveSlot == 0 ? SAVE_SLOT_MAX : _saveSlot - 1;
    }
}

void NdsGameDetailsViewModel::ShowCheats()
{
    StoreSaveSlot();
    _romBrowserController->ShowCheats();
}

void NdsGameDetailsViewModel::Close()
{
    StoreSaveSlot();
    _romBrowserController->HideGameInfo();
}

void NdsGameDetailsViewModel::StoreSaveSlot()
{
    if (!_isLoaded || _saveSlot == _storedSaveSlot)
    {
        return;
    }

    _storedSaveSlot = _saveSlot;

    // The task can outlive this view model
    // Give it its own romPath just in case
    u32 romPathLength = strlen(_romPath) + 1;
    auto romPath = std::make_unique<TCHAR[]>(romPathLength);
    memcpy(romPath.get(), _romPath, romPathLength * sizeof(TCHAR));
    _romBrowserController->GetIoTaskQueue()->Enqueue(
        [saveSlotRepository = &_romBrowserController->GetSaveSlotRepository(),
            romPath = std::move(romPath), saveSlot = _saveSlot] (const vu8& cancelRequested)
        {
            saveSlotRepository->SetSaveSlot(romPath.get(), saveSlot);
            return TaskResult<void>::Completed();
        });
}
