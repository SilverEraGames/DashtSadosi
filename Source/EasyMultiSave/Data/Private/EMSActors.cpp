//Easy Multi Save - Copyright (C) 2026 by Michael Hegemann.  

#include "EMSActors.h"
#include "EMSData.h"  
#include "EMSMisc.h"
#include "EMSLevel.h"
#include "EMSPluginSettings.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelStreamingDynamic.h"

/**
FActorHelpers
**/

FString FActorHelpers::GetActorLevelName(const AActor* Actor)
{
	if (!Actor)
	{
		return FString();
	}

	const UWorld* World = Actor->GetWorld();
	if (!World)
	{
		return FString();
	}

	const ULevel* ActorLevel = Actor->GetLevel();
	if (!ActorLevel || !ActorLevel->GetOuter())
	{
		UE_LOG(LogEasyMultiSave, Warning, TEXT("Failed to return level for Actor: %s"), *Actor->GetName());
		return Actor->GetName();
	}

	const FString ActorLevelName = ActorLevel->GetOuter()->GetName();

	/*
	This supports multiple streaming levels and instances(Also in streaming levels) of the same type. 
	Every streaming level and instance is a ULevelStreamingDynamic. 
	For level instances, this will return the full unique ID. For levels in the 'Levels' list, this appends an index.

	Not backwards compatible with EMS 1.71 and below. 
	Can be easily adapted to old saves if required: 
	#1 Always return 'ActorLevelName + PreFinalString'
	#2 Remove !FStreamHelpers::AutoSaveLoadWorldPartition()
	#3 Eventually remove StripPIEPrefix 
	*/
	if (FSettingHelpers::IsDynamicLevelStreaming() && !FStreamHelpers::AutoSaveLoadWorldPartition())
	{
		FString InstString = FLevelHelpers::GetLevelInstanceNameAsString(Actor);
		const FString FixedInstString = FLevelHelpers::StripPIEPrefix(World, InstString);
		const FString PreFinalString = FixedInstString.Replace(EMS::RuntimeLevelInstance, TEXT(""));

		//Might have a duplicate name, we want to return the minimal string representation of the unique ID.
		const bool bAlreadyHasLevel = InstString.Contains(ActorLevelName);
		const FString FinalString = bAlreadyHasLevel ? PreFinalString : ActorLevelName + PreFinalString;

		return FinalString;
	}

	return ActorLevelName;
}

FString FActorHelpers::GetFullActorName(const AActor* Actor)
{
	const FString ActorName = Actor->GetName();

	//World Partition has own unique Actor Ids
	if (FStreamHelpers::AutoSaveLoadWorldPartition())
	{
		return ActorName;
	}

	//This is only valid for placed Actors. Runtime Actors are always in the persistent.
	//Can't use GetActorType here, since it would crash Multi-Thread loading.
	if (IsPlacedActor(Actor))
	{
		const FString LevelString = FActorHelpers::GetActorLevelName(Actor);
		const bool bAlreadyHas = ActorName.Contains(LevelString);
		const FString FinalString = bAlreadyHas ? ActorName : (LevelString + EMS::UnderscoreInt + ActorName);

		return FinalString;
	}

	return ActorName;
}

FString FActorHelpers::GetComponentName(const AActor* Actor, const UActorComponent* Comp)
{
	if (!Actor)
	{
		return FString();
	}

	const FString CompName = Comp->GetName();

	//Edge case where a component owned by ActorB is saved from ActorA
	const AActor* CompOwner = Comp->GetOwner();
	if (CompOwner && CompOwner != Actor)
	{
		if (CompOwner->IsChildActor())
		{
			return CompOwner->GetParentComponent()->GetName() + CompName;
		}

		return GetFullActorName(CompOwner) + CompName;
	}

	return CompName;
}

FName FActorHelpers::GetActorDataName(const FActorSaveData& ActorData)
{
	const FString ActorStr = FSaveHelpers::StringFromBytes(ActorData.Name);
	const FName ActorName(*ActorStr);
	return ActorName;
}

bool FActorHelpers::IsMovable(const USceneComponent* SceneComp)
{
	if (SceneComp)
	{
		return SceneComp->Mobility == EComponentMobility::Movable;
	}

	return false;
}

bool FActorHelpers::HasValidTransform(const FTransform& CheckTransform)
{
	return CheckTransform.IsValid() && CheckTransform.GetLocation() != FVector::ZeroVector;
}

bool FActorHelpers::CanProcessActorTransform(const AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	//For runtime Actors, we also want to properly place stationary ones
	const bool bCanBeMoved = IsMovable(Actor->GetRootComponent()) || !IsPlacedActor(Actor);

	//Check if Actor is valid, movable, not marked to skip transform, and has no attach parent
	return bCanBeMoved && !IsSkipTransform(Actor) && Actor->GetAttachParentActor() == nullptr;
}

bool FActorHelpers::IsPlacedActor(const AActor* Actor)
{
	return Actor && (Actor->IsNetStartupActor() || Actor->HasAnyFlags(RF_WasLoaded));
}

bool FActorHelpers::IsPersistentActor(const AActor* Actor)
{
	return Actor && Actor->ActorHasTag(EMS::PersistentTag);
}

bool FActorHelpers::IsSkipTransform(const AActor* Actor)
{
	return Actor && Actor->ActorHasTag(EMS::SkipTransformTag);
}

bool FActorHelpers::IsLoaded(const AActor* Actor)
{
	return Actor && Actor->ActorHasTag(EMS::HasLoadedTag);
}

bool FActorHelpers::IsSkipSave(const AActor* Actor)
{
	return Actor && Actor->ActorHasTag(EMS::SkipSaveTag);
}

bool FActorHelpers::IsLevelActor(const EActorType Type, const bool bIncludeScripts)
{
	if (bIncludeScripts && Type == EActorType::AT_LevelScript)
	{
		return true;
	}

	return IsLevelPlaced(Type) || IsRuntime(Type);
}

bool FActorHelpers::IsLevelPlaced(const EActorType Type)
{
	return Type == EActorType::AT_Placed 
		|| Type == EActorType::AT_Destroyed;
}

bool FActorHelpers::IsRuntime(const EActorType Type)
{
	return Type == EActorType::AT_Runtime
		|| Type == EActorType::AT_Persistent;
}

bool FActorHelpers::IsMultiLevelStreamRelevant(const EActorType Type)
{
	return IsLevelPlaced(Type);
}

bool FActorHelpers::IsPersistent(const EActorType Type)
{
	return Type == EActorType::AT_Persistent;
}

bool FActorHelpers::IsLevelScript(const EActorType Type)
{
	return Type == EActorType::AT_LevelScript;
}

void FActorHelpers::SortLevelActors(TArray<FActorSaveData>& ToSort, const APlayerController* PC)
{
	const bool bShouldSort = FSettingHelpers::IsMultiThreadLoading() || FSettingHelpers::IsDeferredLoading();
	if (!bShouldSort)
	{
		return;
	}

	if (PC)
	{
		ToSort.Sort([PC](const FActorSaveData& A, const FActorSaveData& B)
		{
			return FActorHelpers::CompareDistance(A.Transform.GetLocation(), B.Transform.GetLocation(), PC);
		});
	}
}

bool FActorHelpers::CompareDistance(const FVector& VecA, const FVector& VecB, const APlayerController* PC)
{
	if (PC && PC->PlayerCameraManager)
	{
		const FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
		const float DistA = FVector::Dist(VecA, CameraLoc);
		const float DistB = FVector::Dist(VecB, CameraLoc);
		return DistA < DistB;
	}

	return false;
}

void FActorHelpers::SetPlayerNotLoaded(APlayerController* PC)
{
	if (IsValid(PC))
	{
		PC->Tags.Remove(EMS::HasLoadedTag);

		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->Tags.Remove(EMS::HasLoadedTag);
		}

		if (APlayerState* PlayerState = PC->PlayerState)
		{
			PlayerState->Tags.Remove(EMS::HasLoadedTag);
		}
	}
}

EActorType FActorHelpers::GetActorType(const AActor* Actor)
{
	//Runtime spawned
	if (!IsValid(Actor))
	{
		return EActorType::AT_Runtime;
	}

	//Check if the actor is a Pawn and is controlled by a player
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (Pawn->IsPlayerControlled())
		{
			return EActorType::AT_PlayerPawn;
		}
	}

	if (Cast<APlayerController>(Actor) || Cast<APlayerState>(Actor))
	{
		return EActorType::AT_PlayerActor;
	}

	if (Cast<ALevelScriptActor>(Actor))
	{
		return EActorType::AT_LevelScript;
	}

	if (Cast<AGameModeBase>(Actor) || Cast<AGameStateBase>(Actor))
	{
		return EActorType::AT_GameObject;
	}

	if (IsPersistentActor(Actor))
	{
		return EActorType::AT_Persistent;
	}

	if (IsPlacedActor(Actor))
	{
		return EActorType::AT_Placed;
	}

	return EActorType::AT_Runtime;
}

void FActorHelpers::PruneSavedActors(const TMap<FName, const TWeakObjectPtr<AActor>>& InActorMap, TArray<FActorSaveData>& OutSaved)
{
	//Copy array, as it may be accessed from another thread
	TArray<FActorSaveData> PruneArray = OutSaved;

	//Reverse iterate through the array and remove unloaded placed Actors
	for (int32 i = PruneArray.Num() - 1; i >= 0; --i)
	{
		const FActorSaveData ActorArray = PruneArray[i];
		const FName ActorName = FActorHelpers::GetActorDataName(ActorArray);
		const TWeakObjectPtr<AActor> ActorPtr = InActorMap.FindRef(ActorName);

		const bool bValidPtr = ActorPtr.IsValid(false, true);
		const bool bWasLoaded = bValidPtr && ActorPtr.Get() && FActorHelpers::IsLoaded(ActorPtr.Get());

		if (!bValidPtr || bWasLoaded)
		{
			const EActorType Type = EActorType(ActorArray.Type);
			if (FActorHelpers::IsLevelPlaced(Type))
			{
				#if EMS_ENGINE_MIN_UE55
					OutSaved.RemoveAtSwap(i, 1, EAllowShrinking::No);
				#else
					OutSaved.RemoveAtSwap(i, 1, false);
				#endif
			}
		}
	}
}

/**
FSpawnHelpers
**/

UClass* FSpawnHelpers::StaticLoadSpawnClass(const FString& Class)
{
	//Resolve directly
	UClass* ResolvedClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), nullptr, *Class, nullptr, LOAD_None, nullptr));
	if (ResolvedClass)
	{
		return ResolvedClass;
	}

	//Check for Redirects
	auto Redirectors = UEMSPluginSettings::Get()->RuntimeClasses;
	if (!Redirectors.IsEmpty())
	{
		const FSoftClassPath* RedirectedClass = Redirectors.Find(Class);
		if (RedirectedClass)
		{
			return RedirectedClass->TryLoadClass<AActor>();
		}
	}

	return nullptr;
}

UClass* FSpawnHelpers::ResolveSpawnClass(const FString& InClass)
{
	if (InClass.IsEmpty())
	{
		return nullptr;
	}

	UClass* SpawnClass = FindObject<UClass>(nullptr, *InClass);
	if (!SpawnClass)
	{
		SpawnClass = FSpawnHelpers::StaticLoadSpawnClass(InClass);
	}

	return SpawnClass;
}

static FName MakeActorName(const FActorSaveData& ActorArray)
{
	return FName(FSaveHelpers::StringFromBytes(ActorArray.Name));
}

FActorSpawnParameters FSpawnHelpers::GetSpawnParams(const UObject* WorldContext, const FActorSaveData& ActorArray)
{
	FActorSpawnParameters SpawnParams;

	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Name = MakeActorName(ActorArray);
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

	return SpawnParams;
}

AActor* FSpawnHelpers::CheckForExistingActor(const UWorld* InWorld, const FActorSaveData& ActorArray)
{
	if (InWorld && InWorld->PersistentLevel)
	{
		const FName LoadedActorName = MakeActorName(ActorArray);
		AActor* NewLevelActor = Cast<AActor>(StaticFindObjectFast(nullptr, InWorld->PersistentLevel, LoadedActorName));
		if (NewLevelActor)
		{
			return NewLevelActor;
		}
	}

	return nullptr;
}
