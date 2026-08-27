#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "DonkeyCartActor.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UAnimSequence;
class APawn;

/**
 * Eshak + arava bitta actorРґР°. Butun mantiq C++'da:
 *  - oldinga harakat (in-place walk animatsiyasi uchun),
 *  - tezlikРєР° qarab Idle/Walk almashish (single-node playback, AnimBP shart emas),
 *  - g'ildiraklarni haqiqiy tezlikdan aylantirish (bitta Speed manbai),
 *  - aravani eshak orqasiga tow qilish (socket yoki qattiq offset).
 * Hech qanday Blueprint tuguni ulash kerak emas вЂ” meshlarРЅРё Details panelРёРґР°РЅ biriktiring.
 */
UCLASS()
class DASHTSADOSI_API ADonkeyCartActor : public AActor
{
	GENERATED_BODY()

public:
	ADonkeyCartActor();

	virtual void Tick(float DeltaSeconds) override;

	// O'yinchi shu masofa ichida bo'lsa mount qila oladi (sm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Drive", meta = (ClampMin = "0.0"))
	float MountRange = 300.0f;

	// Haydovchi tezligi (grad/soniya) вЂ” WSAD'РґР°gi A/D shuncha tezlikda buriladi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Drive", meta = (ClampMin = "0.0"))
	float TurnRateDegPerSec = 60.0f;

	// NewDriver'ni o'rindiqqa (SeatComponent) biriktiradi va harakatni shu pawn boshqarishiga o'tkazadi.
	UFUNCTION(BlueprintCallable, Category = "DonkeyCart|Drive")
	void MountDriver(APawn* NewDriver);

	// Haydovchini tushiradi, harakatni o'z holatiga (avtomatik yoki hech qanday) qaytaradi.
	UFUNCTION(BlueprintCallable, Category = "DonkeyCart|Drive")
	void DismountDriver();

	UFUNCTION(BlueprintPure, Category = "DonkeyCart|Drive")
	bool IsMounted() const { return DrivingPawn != nullptr; }

	UFUNCTION(BlueprintPure, Category = "DonkeyCart|Drive")
	APawn* GetDrivingPawn() const { return DrivingPawn; }

	// Har frame chaqiriladi (masalan o'yinchining MoveForward'idan): -1..1, WSAD bosilmasa 0.
	UFUNCTION(BlueprintCallable, Category = "DonkeyCart|Drive")
	void SetDriveForwardInput(float Value) { DriveForwardInput = Value; }

	UFUNCTION(BlueprintCallable, Category = "DonkeyCart|Drive")
	void SetDriveTurnInput(float Value) { DriveTurnInput = Value; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// ---------- Komponentlar ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DonkeyCart|Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DonkeyCart|Components")
	USkeletalMeshComponent* DonkeyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DonkeyCart|Components")
	USceneComponent* CartRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DonkeyCart|Components")
	UStaticMeshComponent* CartBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DonkeyCart|Components")
	UStaticMeshComponent* WheelL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DonkeyCart|Components")
	UStaticMeshComponent* WheelR;

	// Haydovchi eshak bilan arava o'rtasidagi taxtga (o'rindiqqa) shu nuqtada o'tqiziladi.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DonkeyCart|Components")
	USceneComponent* SeatComponent;

	// Import qilingan mesh "old" tomoni actor forward (+X) bilan mos kelmasa (masalan mesh
	// +Y bo'ylab modellangan), shu bilan tuzating. Har xil creature rig har xil bo'lishi mumkin вЂ”
	// Play qilib ko'rib to'g'ri qiymatni tanlang.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Mesh")
	FRotator DonkeyMeshRotationOffset = FRotator::ZeroRotator;

	// ---------- Animatsiyalar (Details panelРёРґР°РЅ biriktiring) ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Animation")
	UAnimSequence* WalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Animation")
	UAnimSequence* IdleAnimation;

	// ---------- Harakat ----------
	// Demo/test uchun avtomatik oldinga yurish (hech kim mount qilmagan bo'lsa ham).
	// Odatda FALSE вЂ” haydovchi mount qilib, WSAD bosmasa arava qimirlamaydi.
	// Kimdir mount qilgan bo'lsa (DrivingPawn != nullptr), bu flag e'tiborga olinmaydi вЂ”
	// harakat to'liq SetDriveForwardInput/SetDriveTurnInput orqali boshqariladi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Movement")
	bool bAutoMove = false;

	// sm/s (Unreal birligi). Real eshak yurishi ~100-150 sm/s.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Movement", meta = (ClampMin = "0.0"))
	float MoveSpeed = 120.0f;

	// Shu tezlikdan tez bo'lsa Walk, aks holda Idle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeedThreshold = 5.0f;

	// ---------- G'ildirak ----------
	// WheelL radiusi (sm), g'ildirak meshidan o'lchang. Aylanish tezligi shunga bog'liq.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Wheels", meta = (ClampMin = "0.1"))
	float WheelRadius = 30.0f;

	// WheelR radiusi. Old/orqa g'ildirak diametri farq qilsa (masalan vagon), har biri
	// o'z chin radiusiga mos aylanishi uchun alohida qiymat вЂ” aks holda kattaroq g'ildirak
	// "sirg'anayotgandek" tez aylanadi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Wheels", meta = (ClampMin = "0.1"))
	float WheelRadiusR = 30.0f;

	// Aylanish o'qi import orientatsiyasiga bog'liq вЂ” Play'РґР° ko'rib to'g'risini tanlang.
	// EAxis::Y = Pitch (odatiy), X = Roll, Z = Yaw.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Wheels")
	TEnumAsByte<EAxis::Type> WheelSpinAxis = EAxis::Y;

	// Teskari aylansa вЂ” belgini o'zgartiradi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Wheels")
	bool bReverseWheelSpin = false;

	// ---------- Tow (aravani ulash) ----------
	// True bo'lsa: arava SK_Donkey'РґР°РіРё SocketName socketiga har frame yopishadi (tabiiy tebranish).
	// False bo'lsa: arava actorga qattiq offset bilan orqada turadi (barqaror).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Tow")
	bool bAttachToSocket = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Tow")
	FName SocketName = "CartSocket";

	// Socket ishlatilmaganРґР° aravaning eshakРґР°РЅ orqР°РґР°РіРё offseti (local, sm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Tow")
	FVector CartOffset = FVector(-120.0f, 0.0f, 0.0f);

	// Socket'ga yumshoq ergashish tezligi (0 = qattiq). Katta = tezroq moslashadi.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DonkeyCart|Tow", meta = (ClampMin = "0.0"))
	float TowInterpSpeed = 12.0f;

private:
	FVector PreviousLocation = FVector::ZeroVector;
	bool bIsWalking = false;

	UPROPERTY()
	APawn* DrivingPawn = nullptr;

	float DriveForwardInput = 0.0f;
	float DriveTurnInput = 0.0f;
	TEnumAsByte<EMovementMode> PreMountMovementMode = MOVE_Walking;

	void ApplyWheelSpin(float Speed, float DeltaSeconds);
	void UpdateAnimationState(float Speed);
	void UpdateCartTow(float DeltaSeconds);
	void UpdateDrivenMovement(float DeltaSeconds);
	FRotator MakeAxisRotation(float DegreesToAdd) const;
};

