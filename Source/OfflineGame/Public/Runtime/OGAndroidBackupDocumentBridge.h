#pragma once

#include "CoreMinimal.h"

/**
 * Android Storage Access Framework bridge for uninstall-safe world backup
 * export/import. The platform picker owns the external URI; the canonical
 * world remains internal and is copied only after explicit user selection.
 */
class OFFLINEGAME_API FOGAndroidBackupDocumentBridge
{
public:
    static bool LaunchExport(
        const FString& SourceBackupPath,
        const FString& SuggestedFileName,
        FString& OutError);

    static bool LaunchImport(
        const FString& InternalDestinationPath,
        FString& OutError);

    /** Choose a writable Android SAF tree for rotating external snapshots. */
    static bool ChooseAutomaticBackupFolder(FString& OutError);
    static bool ShouldQueueAutomaticBackup();
    static bool QueueAutomaticBackup(const FString& SourcePath, const FString& Name, FString& OutError);
    static FName GetAutomaticBackupState();

    /** Manual picker transfer state and selected URI or error detail. */
    static FName GetTransferState(
        FString& OutDetail);
};
