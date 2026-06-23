//Easy Multi Save - Copyright (C) 2026 by Michael Hegemann.  

#pragma once

#include "EMSObjectBase.h"
#include "Engine/LevelStreamingDynamic.h"
#include "EMSObject.generated.h"

class UEMSCustomSaveGame;
class UEMSInfoSaveGame;
class UEMSAsyncSaveGame;
class UEMSAsyncLoadGame;
class FBufferArchive;
class FMemoryReader;
class APawn;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEmsLoadPlayerComplete, const APlayerController*, LoadedPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEmsLoadLevelComplete, const TArray<TSoftObjectPtr<AActor>>&, LoadedActors);

UCLASS(BlueprintType, meta = (DisplayName = "Easy Multi Save", Keywords = "Save, EMS, EasyMultiSave, EasySave"))
class EASYMULTISAVE_API UEMSObject : public UEMSObjectBase
{
	GENERATED_BODY()

protected:
	UEMSObject();

/** Variables */

public:

	UPROPERTY(BlueprintAssignable, Category = "Easy Multi Save | Delegates")
	FEmsLoadPlayerComplete OnPlayerLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Easy Multi Save | Delegates")
	FEmsLoadLevelComplete OnLevelLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Easy Multi Save | Delegates")
	FEmsLoadLevelComplete OnPartitionLoaded;

private:

	FCriticalSection SaveActorsScope;
	FCriticalSection LoadActorScope;

	FName CachedPersistentLevelName;

	uint8 bLoadFromMemory : 1;
	uint8 bInitWorldPartitionLoad : 1;
	uint8 bLoadPartition : 1;
	uint8 bSavePartition : 1;
	uint8 bCachedIsWorldPartition : 1;

	float WorldPartitionInitTimer;

	FDelegateHandle ActorDestroyedHandle;

/** Data Archive Cache Variables, Zero initialized(Transient) */

private:

	//Using this instead of Actor iterator v1.78+
//	UPROPERTY(Transient)
//	TArray<TWeakObjectPtr<AActor>> WorldActors;

	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<AActor>> ActorList;

	UPROPERTY(Transient)
	TMap<FName, const TWeakObjectPtr<AActor>> ActorMap;

private:

	UPROPERTY(Transient)
	TArray<FLevelArchive> LevelArchiveList;	

	UPROPERTY(Transient)
	FMultiLevelStreamingData MultiLevelStreamData;
	
	UPROPERTY(Transient)
	TArray<FActorSaveData> SavedActors;

	UPROPERTY(Transient)
	TArray<FActorSaveData> SavedActorsPruned;

	UPROPERTY(Transient)
	TArray<FLevelScriptSaveData> SavedScripts;

	UPROPERTY(Transient)
	FGameObjectSaveData SavedGameMode;

	UPROPERTY(Transient)
	FGameObjectSaveData SavedGameState;

private:

	UPROPERTY(Transient)
	TSet<FActorSaveData> WorldPartitionActors;

	UPROPERTY(Transient)
	TSet<FActorSaveData> DestroyedActors;

	UPROPERTY(Transient)
	mutable TMap<TWeakObjectPtr<AActor>, FGameObjectSaveData> RawObjectData;

	UPROPERTY(Transient)
	mutable TArray<TSoftObjectPtr<AActor>> RealLoadedActors;

private:

	UPROPERTY(Transient)
	FPlayerStackArchive PlayerStackData;

	UPROPERTY(Transient)
	FPlayerArchive SavedPlayer;

/** Blueprint Library function accessors */
	
public:

	UObject* LoadRawObject(AActor* Actor, const FRawObjectSaveData& Data) const;
	bool SaveRawObject(AActor* Actor, const FRawObjectSaveData& Data) const;

	bool SavePlayerActorsCustom(AController* Controller, const FString& FileName);
	bool LoadPlayerActorsCustom(AController* Controller, const FString& FileName);
	bool DeleteCustomPlayerFile(const FString& FileName);

/** Other public Functions  */

public:

	static UEMSObject* Get(const UObject* WorldContextObject);

	void PrepareLoadAndSaveActors(const uint32 Data, const EAsyncCheckType FunctionType, const EPrepareType PrepareType);

	bool SavePlayerActors(APlayerController* Controller, const FString& FileName);
	void LoadPlayerActors(APlayerController* Controller);

	bool SaveLevelActors(const bool bMemoryOnly);
	void LoadLevelActors(UEMSAsyncLoadGame* LoadTask);
	void LoadGameMode();
	void LoadLevelScripts();

	void FinishLoadingLevel(const bool bHasLoadedFile);

	bool TryLoadPlayerFile();
	bool TryLoadLevelFile();

	APlayerController* GetPlayerController() const;
	APawn* GetPlayerPawn(const APlayerController* PC) const;

	FTimerManager& GetTimerManager() const;

	bool HasValidGameMode() const;
	bool HasValidPlayer() const;

/** Internal Functions  */

public:

	void SaveActorToBinary(AActor* Actor, FGameObjectSaveData& OutData) const;
	void LoadActorFromBinary(AActor* Actor, const FGameObjectSaveData& InData) const;

	void SpawnLevelActor(const FActorSaveData& ActorArray) const;
	void ProcessLevelActor(AActor* Actor, const FActorSaveData& ActorArray) const;

	void PrepareLevelActors();

protected:

	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

private:

	void CreateLevelActor(UClass* SpawnClass, const FActorSaveData& ActorArray, const FActorSpawnParameters& SpawnParams) const;
	void FailSpawnLevelActor(const FActorSaveData& ActorArray) const;

	bool UnpackActorArchive(const EDataLoadType LoadType, FMemoryReader& FromBinary) override;
	bool UnpackLevelArchive(FMemoryReader& FromBinary);
	bool UnpackPlayerArchive(FMemoryReader& FromBinary);
	bool UnpackLevel(const FLevelArchive& LevelArchive);
	void UnpackPlayer(const FPlayerArchive& PlayerArchive);

	void PrepareLoadActor(const uint32 Data, AActor* Actor, const bool bFullReload);
	void PrepareSaveActor(const EPrepareType PrepareType, AActor* Actor);
	void PrepareFullReload(const uint32 Data, AActor* Actor) const;
	void AddActorToList(AActor* Actor, const bool bIsLoading);
	
	FGameObjectSaveData ParseGameModeObjectForSaving(AActor* Actor) const;
	FLevelScriptSaveData ParseLevelScriptForSaving(AActor* Actor) const;
	FActorSaveData ParseLevelActorForSaving(AActor* Actor, const EActorType Type) const;

	static void ProcessSaveEvent(TWeakObjectPtr<UObject> WeakRef, ESaveEventType SaveType);
	void ExecuteSaveEvent(UObject* InObject, const ESaveEventType SaveType) const;
	void ExecuteActorPreSave(AActor* Actor) const;
	void ExecuteActorSaved(AActor* Actor) const;
	void ExecuteActorPreLoad(AActor* Actor) const;
	void ExecuteActorLoaded(AActor* Actor) const;
	void ExecuteComponentPreSave(UActorComponent* Comp) const;
	void ExecuteComponentSaved(UActorComponent* Comp) const;

	void SerializeActorStructProperties(AActor* Actor) const;

	TArray<UActorComponent*> GetSaveComponents(AActor* Actor) const;
	void SaveActorComponents(AActor* Actor, TArray<FComponentSaveData>& OutComponents) const;
	void LoadActorComponents(AActor* Actor, const TArray<FComponentSaveData>& InComponents) const;

	UObject* SerializeFromRawObject(AActor* Actor, const FRawObjectSaveData& Data, const TArray<FComponentSaveData>& InputArray) const;
	void AppendRawObjectData(AActor* Actor, TArray<FComponentSaveData>& OutComponents) const;
	void UpdateRawObjectData(AActor* Actor, const FComponentSaveData& InputData) const;

	FLevelStackArchive AddMultiLevelStackData(const FLevelArchive& LevelArchive, const FLevelArchive& PersistentArchive, const FGameObjectSaveData& InGameMode, const FGameObjectSaveData& InGameState);
	FLevelArchive AddMultiLevelStreamData(const FLevelArchive& LevelArchive);
	void UpdateMultiLevelStreamData(const FLevelArchive& LevelArchive);

/** Actor Delegates  */

private:

	void OnActorDestroyed(AActor* DestroyedActor);
	void RemoveActorDelegates(UWorld* World);

/** World Delegates  */

	void OnPreWorldInit(UWorld* World, const UWorld::InitializationValues IVS);
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void RemoveWorldPartitionStreamDelegates();

/** World Partition Functions  */

private:

	void OnLevelStreamingStateChanged(UWorld* InWorld, const ULevelStreaming* InStreamingLevel, ULevel* InLevelIfLoaded, ELevelStreamingState PreviousState, ELevelStreamingState NewState);
	void OnLevelBeginMakingInvisible(UWorld* InWorld, const ULevelStreaming* InStreamingLevel, ULevel* InLoadedLevel);

	void TryInitWorldPartition();
	void PollInitWorldPartition();

	void TrySaveWorldPartition();
	void AccumulatedSaveWorldPartition();

public:

	void LoadStreamingActor(AActor* Actor, const FActorSaveData& ActorData);

/** Delegate Functions  */

public:

	inline void BroadcastOnPartitionLoaded()
	{
		OnPartitionLoaded.Broadcast(RealLoadedActors);
		RealLoadedActors.Empty();
	}

	inline void BroadcastOnLevelLoaded()
	{
		OnLevelLoaded.Broadcast(RealLoadedActors);
		RealLoadedActors.Empty();
	}

	inline void AllocateRealLoadedActors(const int32 AllocNum)
	{
		RealLoadedActors.Reserve(AllocNum);
	}

	inline bool HasActuallyLoadedActors()
	{
		return !EMS::ArrayEmpty(RealLoadedActors);
	}

/** Clear Data Functions  */

public:

	inline void ClearMultiLevelSave()
	{
		ClearSavedLevelActors();
		ClearStreamingData();
		ClearMultiSaveLevels();
		LevelArchiveList.Empty();
		PlayerStackData = FPlayerStackArchive();
		bLoadFromMemory = false;
	}

	inline void ClearWorldPartition()
	{
		WorldPartitionActors.Empty();
		bSavePartition = false;
		bLoadPartition = false;
		bInitWorldPartitionLoad = false;
		bLoadFromMemory = false;
		WorldPartitionInitTimer = 0.f;
	}

private:

	inline void ClearPlayerPosition()
	{
		SavedPlayer.ClearPosition();
	}

	inline void ClearActorList()
	{
		ActorList.Empty();
		ActorMap.Empty();
	}

	inline void ClearSavedLevelActors()
	{
		SavedActors.Empty();
		SavedScripts.Empty();
		SavedActorsPruned.Empty();

		ClearDestroyedActors();
	}

	inline void ClearStreamingData()
	{
		MultiLevelStreamData = FMultiLevelStreamingData();
	}

	inline void ClearWorldPartitionActors()
	{
		WorldPartitionActors.Empty();
	}

	inline void ClearDestroyedActors()
	{
		DestroyedActors.Empty();
	}

	inline void ClearUserData() override
	{
		Super::ClearUserData();

		//When setting/deleting a Save User, we need to clear this
		ClearMultiLevelSave();
		ClearWorldPartition();
	}

/** Actor Helpers  */

public:

	static FName LevelScriptSaveName(const AActor* Actor);
	static EActorType GetActorType(const AActor* Actor);
	static FString GetFullActorName(const AActor* Actor);
	static bool IsLoaded(const AActor* Actor);
	static bool IsSkipSave(const AActor* Actor);
	static bool IsSkipTransform(const AActor* Actor);
	static bool IsLevelScript(const EActorType Type);
	static bool HasSaveInterface(const AActor* Actor);
	static bool HasComponentSaveInterface(const UActorComponent* Comp);

protected:

	FName GetLevelName() const override;
	bool IsLevelName(const FName InCompareName) const;
	void ClearLoadFromMemory() override;

/** Other Helper Functions  */

public:

	inline static bool AutoDestroyActors()
	{
		return UEMSPluginSettings::Get()->bAutoDestroyActors;
	}

	inline static bool AdvancedSpawnCheck()
	{
		return UEMSPluginSettings::Get()->bAdvancedSpawnCheck;
	}

	inline bool IsValidActor(const AActor* Actor) const
	{
		return IsValid(Actor) && HasSaveInterface(Actor);
	}

	inline bool IsValidForSaving(const AActor* Actor) const
	{
		return IsValidActor(Actor) && !IsSkipSave(Actor);
	}

	inline bool IsValidForLoading(const AActor* Actor) const
	{
		return IsValidActor(Actor) && !IsLoaded(Actor);
	}

	inline FMultiLevelStreamingData GetMultiLevelStreamData() const
	{
		return MultiLevelStreamData;
	}

	inline bool HasStreamingLevels()
	{
		return FStreamHelpers::HasStreamingLevels(GetWorld());
	}

	inline bool HasLevelData() const
	{
		return !EMS::ArrayEmpty(SavedActors) 
			|| !EMS::ArrayEmpty(SavedScripts) 
			|| !EMS::ArrayEmpty(SavedGameMode.Data)
			|| !EMS::ArrayEmpty(SavedGameState.Data)
			|| MultiLevelStreamData.HasData();
	}

/** World Partition Helpers  */

public:

	inline bool AutoSaveLoadWorldPartition() const
	{
		return bCachedIsWorldPartition;
	}

	inline bool CanProcessWorldPartition() const
	{
		return FStreamHelpers::CanProcessWorldPartition(GetWorld());
	}

	inline bool IsInitialWorldPartitionLoading() const
	{
		return !bInitWorldPartitionLoad && IsAsyncSaveOrLoadTaskActive(ESaveGameMode::MODE_Level, EAsyncCheckType::CT_Load, false);
	}

	inline bool IsLevelStreaming() const
	{
		return FStreamHelpers::IsLevelStillStreaming(GetWorld());
	}

	inline static bool SkipInitialWorldPartitionLoad()
	{
		return UEMSPluginSettings::Get()->WorldPartitionInit == EWorldPartitionInit::Skip;
	}

	inline bool AlwaysAutoLoadWorldPartition() const
	{
		return bInitWorldPartitionLoad && bLoadFromMemory;
	}

	inline bool InitWorldPartitionLoadComplete() const
	{
		return bInitWorldPartitionLoad && !IsLevelStreaming() && !IsInitialWorldPartitionLoading();
	}
};
