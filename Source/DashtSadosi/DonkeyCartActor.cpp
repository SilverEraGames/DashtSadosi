#include "DonkeyCartActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ADonkeyCartActor::ADonkeyCartActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Eshak вЂ” root'ga bevosita.
	DonkeyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DonkeyMesh"));
	DonkeyMesh->SetupAttachment(SceneRoot);

	// Arava root'i вЂ” root'ga (bone'ga emas). Tow logikasi joyini boshqaradi.
	CartRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CartRoot"));
	CartRoot->SetupAttachment(SceneRoot);

	CartBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartBody"));
	CartBody->SetupAttachment(CartRoot);

	// G'ildiraklar вЂ” CartRoot ostida. Har birining pivoti mesh markazРёРґР° bo'lsin (import'РґР°).
	WheelL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelL"));
	WheelL->SetupAttachment(CartRoot);

	WheelR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelR"));
	WheelR->SetupAttachment(CartRoot);

	// Standart joylashuv вЂ” aravaning oldingi-yuqori qismi (eshak bilan qo'shilgan taxta atrofi).
	// Aniq mesh o'lchamiga qarab Details panelРёРґР°РЅ yoki instansiyada sozlang.
	SeatComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SeatComponent"));
	SeatComponent->SetupAttachment(CartRoot);
	SeatComponent->SetRelativeLocation(FVector(130.0f, 0.0f, 110.0f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HorseMeshFinder(
		TEXT("/Game/HorseCart/Horse/HORSE_DEMO.HORSE_DEMO"));
	if (HorseMeshFinder.Succeeded())
	{
		DonkeyMesh->SetSkeletalMesh(HorseMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CartBodyFinder(
		TEXT("/Game/HorseCart/Cart/SM_HorseCart_Body.SM_HorseCart_Body"));
	if (CartBodyFinder.Succeeded())
	{
		CartBody->SetStaticMesh(CartBodyFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WheelFrontFinder(
		TEXT("/Game/HorseCart/Cart/SM_HorseCart_WheelFront.SM_HorseCart_WheelFront"));
	if (WheelFrontFinder.Succeeded())
	{
		WheelL->SetStaticMesh(WheelFrontFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WheelRearFinder(
		TEXT("/Game/HorseCart/Cart/SM_HorseCart_WheelRear.SM_HorseCart_WheelRear"));
	if (WheelRearFinder.Succeeded())
	{
		WheelR->SetStaticMesh(WheelRearFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimationFinder(
		TEXT("/Game/HorseCart/Horse/HORSE_DEMOHorse_Horse_Idle.HORSE_DEMOHorse_Horse_Idle"));
	if (IdleAnimationFinder.Succeeded())
	{
		IdleAnimation = IdleAnimationFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimationFinder(
		TEXT("/Game/HorseCart/Horse/HORSE_DEMOHorse_Horse_Walk.HORSE_DEMOHorse_Horse_Walk"));
	if (WalkAnimationFinder.Succeeded())
	{
		WalkAnimation = WalkAnimationFinder.Object;
	}
}

void ADonkeyCartActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Socket ishlatilmasa вЂ” aravani boshlang'ich offsetРіР° qo'yamiz (editorРґР° ko'rinishi uchun).
	if (!bAttachToSocket && CartRoot)
	{
		CartRoot->SetRelativeLocation(CartOffset);
	}

	if (DonkeyMesh)
	{
		DonkeyMesh->SetRelativeRotation(DonkeyMeshRotationOffset);
	}
}

void ADonkeyCartActor::BeginPlay()
{
	Super::BeginPlay();

	PreviousLocation = GetActorLocation();

	// AnimBP shart emas вЂ” bitta anim sequence'ni to'g'ridan-to'g'ri o'ynatamiz.
	if (DonkeyMesh)
	{
		DonkeyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (IdleAnimation)
		{
			DonkeyMesh->PlayAnimation(IdleAnimation, true);
			bIsWalking = false;
		}
	}
}

void ADonkeyCartActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	if (DrivingPawn)
	{
		// Haydovchi mount qilgan вЂ” harakat to'liq uning WSAD kirishidan.
		UpdateDrivenMovement(DeltaSeconds);
	}
	else if (bAutoMove && MoveSpeed > 0.0f)
	{
		// Demo/test rejimi вЂ” hech kim mount qilmagan bo'lsa ham oldinga yuradi.
		AddActorWorldOffset(GetActorForwardVector() * MoveSpeed * DeltaSeconds, false);
	}

	// BITTA Speed manbai: haqiqiy joylashuv o'zgarishi. Animatsiya ham, g'ildirak ham shundan.
	const FVector CurrentLocation = GetActorLocation();
	const float Speed = FVector::Dist(CurrentLocation, PreviousLocation) / DeltaSeconds;
	PreviousLocation = CurrentLocation;

	UpdateAnimationState(Speed);
	ApplyWheelSpin(Speed, DeltaSeconds);
	UpdateCartTow(DeltaSeconds);
}

void ADonkeyCartActor::UpdateAnimationState(float Speed)
{
	if (!DonkeyMesh)
	{
		return;
	}

	const bool bShouldWalk = Speed > WalkSpeedThreshold;
	if (bShouldWalk == bIsWalking)
	{
		return; // holat o'zgarmadi вЂ” animatsiyani qayta boshlamaymiz
	}

	bIsWalking = bShouldWalk;
	UAnimSequence* Next = bShouldWalk ? WalkAnimation : IdleAnimation;
	if (Next)
	{
		DonkeyMesh->PlayAnimation(Next, true);
	}
}

FRotator ADonkeyCartActor::MakeAxisRotation(float Deg) const
{
	// FRotator(Pitch, Yaw, Roll): Pitch=Y o'qi, Yaw=Z o'qi, Roll=X o'qi.
	switch (WheelSpinAxis)
	{
	case EAxis::X: return FRotator(0.0f, 0.0f, Deg); // Roll
	case EAxis::Z: return FRotator(0.0f, Deg, 0.0f); // Yaw
	default:       return FRotator(Deg, 0.0f, 0.0f); // Pitch (EAxis::Y)
	}
}

void ADonkeyCartActor::ApplyWheelSpin(float Speed, float DeltaSeconds)
{
	auto SpinWheel = [this, Speed, DeltaSeconds](UStaticMeshComponent* Wheel, float Radius)
	{
		if (!Wheel || Radius <= 0.0f)
		{
			return;
		}
		const float Circumference = 2.0f * PI * Radius;
		const float DegPerSec = (Speed / Circumference) * 360.0f;
		float DeltaDeg = DegPerSec * DeltaSeconds;
		if (bReverseWheelSpin)
		{
			DeltaDeg = -DeltaDeg;
		}
		Wheel->AddLocalRotation(MakeAxisRotation(DeltaDeg));
	};

	// Har g'ildirak o'z chin radiusi bilan aylanadi вЂ” bitta Speed manbaidan, lekin turli
	// diametrli g'ildiraklar (masalan old/orqa aksle) turli burchak tezligida aylanishi kerak,
	// aks holda kattaroq g'ildirak "sirg'anayotgandek" ko'rinadi.
	SpinWheel(WheelL, WheelRadius);
	SpinWheel(WheelR, WheelRadiusR);
}

void ADonkeyCartActor::UpdateCartTow(float DeltaSeconds)
{
	if (!CartRoot || !DonkeyMesh)
	{
		return;
	}

	if (bAttachToSocket)
	{
		// Socket mavjud bo'lsa вЂ” uning world-transformРёРіР° yumshoq (yoki qattiq) ergashamiz.
		if (DonkeyMesh->DoesSocketExist(SocketName))
		{
			const FTransform Target = DonkeyMesh->GetSocketTransform(SocketName);
			if (TowInterpSpeed <= 0.0f)
			{
				CartRoot->SetWorldLocationAndRotation(Target.GetLocation(), Target.GetRotation());
			}
			else
			{
				const FVector NewLoc = FMath::VInterpTo(
					CartRoot->GetComponentLocation(), Target.GetLocation(), DeltaSeconds, TowInterpSpeed);
				const FRotator NewRot = FMath::RInterpTo(
					CartRoot->GetComponentRotation(), Target.Rotator(), DeltaSeconds, TowInterpSpeed);
				CartRoot->SetWorldLocationAndRotation(NewLoc, NewRot);
			}
		}
	}
	else
	{
		// Socketsiz вЂ” arava actorga qattiq offset bilan biriktirilgan (relative). Hech narsa kerak emas;
		// OnConstruction'РґР° offset qo'yilgan va CartRoot root'РіР° bog'langani uchun avtomatik ergashadi.
	}
}

void ADonkeyCartActor::UpdateDrivenMovement(float DeltaSeconds)
{
	if (!FMath::IsNearlyZero(DriveTurnInput))
	{
		AddActorWorldRotation(FRotator(0.0f, DriveTurnInput * TurnRateDegPerSec * DeltaSeconds, 0.0f));
	}

	if (!FMath::IsNearlyZero(DriveForwardInput))
	{
		const float ClampedInput = FMath::Clamp(DriveForwardInput, -1.0f, 1.0f);
		AddActorWorldOffset(GetActorForwardVector() * ClampedInput * MoveSpeed * DeltaSeconds, true);
	}
}

void ADonkeyCartActor::MountDriver(APawn* NewDriver)
{
	if (!NewDriver || DrivingPawn || !SeatComponent)
	{
		return;
	}

	DrivingPawn = NewDriver;
	DriveForwardInput = 0.0f;
	DriveTurnInput = 0.0f;

	if (ACharacter* DriverCharacter = Cast<ACharacter>(NewDriver))
	{
		if (UCharacterMovementComponent* Movement = DriverCharacter->GetCharacterMovement())
		{
			PreMountMovementMode = Movement->MovementMode;
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_None);
		}
		if (UCapsuleComponent* Capsule = DriverCharacter->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld, true);
	NewDriver->AttachToComponent(SeatComponent, AttachRules);
	NewDriver->SetActorRelativeLocation(FVector::ZeroVector);
	NewDriver->SetActorRelativeRotation(FRotator::ZeroRotator);
}

void ADonkeyCartActor::DismountDriver()
{
	if (!DrivingPawn)
	{
		return;
	}

	APawn* OldDriver = DrivingPawn;
	DrivingPawn = nullptr;
	DriveForwardInput = 0.0f;
	DriveTurnInput = 0.0f;

	OldDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (ACharacter* DriverCharacter = Cast<ACharacter>(OldDriver))
	{
		if (UCapsuleComponent* Capsule = DriverCharacter->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		if (UCharacterMovementComponent* Movement = DriverCharacter->GetCharacterMovement())
		{
			Movement->SetMovementMode(PreMountMovementMode);
		}
	}

	// Aravaning yon tomoniga tushiradi вЂ” o'rindiq/arava mesh ichiga qolib ketmasin.
	const FVector DismountLocation = SeatComponent
		? SeatComponent->GetComponentLocation() + GetActorRightVector() * 150.0f + FVector(0.0f, 0.0f, 20.0f)
		: GetActorLocation();
	OldDriver->SetActorLocation(DismountLocation, false, nullptr, ETeleportType::TeleportPhysics);
}


