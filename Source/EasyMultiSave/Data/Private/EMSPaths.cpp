//Easy Multi Save - Copyright (C) 2026 by Michael Hegemann.  

#include "EMSPaths.h"
#include "EMSMisc.h"
#include "EMSData.h"
#include "EMSPluginSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "ImageUtils.h"
#include "ImageCore.h"

/**
FSavePaths
**/

TArray<FString> FSavePaths::GetDefaultSaveFiles(const FString& SaveGameName)
{
	TArray<FString> AllFiles;

	using namespace EMS;
	{
		const FString PlayerFile = SaveGameName + UnderscoreFile + PlayerSuffix;
		const FString LevelFile = SaveGameName + UnderscoreFile + ActorSuffix;
		const FString SlotFile = SaveGameName + UnderscoreFile + SlotSuffix;
		const FString ThumbFile = SaveGameName + UnderscoreFile + ThumbSuffix;

		AllFiles.Add(PlayerFile);
		AllFiles.Add(LevelFile);
		AllFiles.Add(SlotFile);
		AllFiles.Add(ThumbFile);
	}

	return AllFiles;
}

FString FSavePaths::GetBackupSavePath(const FString& FullSavePath)
{
	FString SavePath = FullSavePath;

	//Uses prefix
	if (FSettingHelpers::IsConsoleFileSystem())
	{
		const FString BackupTag = EMS::BackupTag + EMS::UnderscoreFile;
		SavePath = BackupTag + FullSavePath;

		return SavePath;
	}

	//Uses suffix, find last slash(folder)
	const int32 SlashIndex = SavePath.Find(EMS::Slash, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (SlashIndex != INDEX_NONE)
	{
		const FString BackupTag = EMS::UnderscoreFile + EMS::BackupTag;
		const FString Directory = SavePath.Left(SlashIndex) + BackupTag;
		const FString FileName = SavePath.Mid(SlashIndex + 1);
		SavePath = Directory + EMS::Slash + FileName;

		return SavePath;
	}

	//Same dir, just append tag
	SavePath = FullSavePath + EMS::UnderscoreFile + EMS::BackupTag;
	
	return SavePath;
}

FString FSavePaths::GetBackupFileName(const FString& BaseName)
{
	if (FSettingHelpers::IsConsoleFileSystem())
	{
		return EMS::BackupTag + EMS::UnderscoreFile + BaseName;
	}

	return BaseName + EMS::UnderscoreFile + EMS::BackupTag;
}

TArray<FString> FSavePaths::GetSortedSaveSlots(TArray<FSaveSlotInfo>& SaveSlots)
{
	SaveSlots.Sort([](const FSaveSlotInfo& A, const FSaveSlotInfo& B)
	{
		return A.TimeStamp > B.TimeStamp;
	});

	TArray<FString> SaveSlotNames;
	for (const FSaveSlotInfo& SlotInfo : SaveSlots)
	{
		//Skip backup tag and user dir
		if (SlotInfo.Name.Contains(EMS::BackupTag) || SlotInfo.Name.Contains(EMS::UserDir))
		{
			continue;
		}

		SaveSlotNames.Add(SlotInfo.Name);
	}

	return SaveSlotNames;
}

FString FSavePaths::ValidateSaveName(const FString& SaveGameName)
{
	FString CurrentSave = SaveGameName;

	//Advanced file checks for console
	if (FSettingHelpers::IsConsoleFileSystem())
	{
		//Get default suffixes 
		const TArray<FString> DefaultSuffixes =
		{
			EMS::PlayerSuffix,
			EMS::ActorSuffix,
			EMS::SlotSuffix,
			EMS::ThumbSuffix
		};

		//Check and remove default suffixex(case-insensitive)
		for (const FString& Suffix : DefaultSuffixes)
		{
			if (CurrentSave.Contains(Suffix, ESearchCase::IgnoreCase))
			{
				UE_LOG(LogEasyMultiSave, Warning, TEXT("Save name '%s' contains reserved suffix '%s' "), *SaveGameName, *Suffix);
				break;
			}
		}

		CurrentSave.TrimEndInline();

		//Special case where we replace all underscores for file name on PS
		CurrentSave = CurrentSave.Replace(TEXT("_"), *EMS::UnderscoreFile);
	}

	CurrentSave = CurrentSave.Replace(TEXT(" "), *EMS::UnderscoreFile);
	CurrentSave = CurrentSave.Replace(TEXT("."), *EMS::UnderscoreFile);

	return FPaths::MakeValidFileName(*CurrentSave);
}

FString FSavePaths::GetThumbnailFormat()
{
	if (UEMSPluginSettings::Get()->ThumbnailFormat == EThumbnailImageFormat::Png)
	{
		return EMS::ImgFormatPNG;
	}

	return EMS::ImgFormatJPG;
}

FString FSavePaths::GetThumbnailFileExtension()
{
	if (FSettingHelpers::IsConsoleFileSystem())
	{
		return EMS::SaveType;
	}

	return TEXT(".") + GetThumbnailFormat();
}

void FSavePaths::CheckForReadOnly(const FString& FullSavePath)
{
	const FString NativePath = FString::Printf(EMS::NativeDesktopSavePath, *FPaths::ProjectSavedDir(), *FullSavePath);
	const bool bReadOnly = IFileManager::Get().IsReadOnly(*NativePath);
	if (bReadOnly)
	{
		FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*NativePath, false);
		UE_LOG(LogEasyMultiSave, Warning, TEXT("File access read only was set to false: %s"), *NativePath);
	}
}

TArray<FString> FSavePaths::GetConsoleSlotFiles(const TArray<FString>& SaveGameNames)
{
	const FString FullSlotSuffix = EMS::UnderscoreFile + EMS::SlotSuffix;

	//Filter out slots
	TArray<FString> SlotNames;
	for (const FString& ActualFileName : SaveGameNames)
	{
		const bool bIsActualSlot = ActualFileName.Contains(FullSlotSuffix);
		if (bIsActualSlot)
		{
			//Get actual name without suffix
			const int32 Index = ActualFileName.Find(FullSlotSuffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd, INDEX_NONE);
			const int32 Count = FullSlotSuffix.Len();

			FString ReducedFileName = ActualFileName;
			ReducedFileName.RemoveAt(Index, Count);
			SlotNames.Add(ReducedFileName);
		}
	}

	return SlotNames;
}

/**
Thumbnail Functions
**/

bool FSaveThumbnails::HasRenderTargetResource(UTextureRenderTarget2D* TextureRenderTarget)
{
	return TextureRenderTarget->GetResource() != nullptr;
}

bool FSaveThumbnails::CompressImage(const FImage& Image, FArchive& Ar)
{
	TArray64<uint8> CompressedData;

	//The actual compression is CPU-bound and perfectly safe for background threads
	if (!FImageUtils::CompressImage(CompressedData, *FSavePaths::GetThumbnailFormat(), Image, 90))
	{
		return false;
	}

	Ar.Serialize(CompressedData.GetData(), CompressedData.Num());

	return true;
}