#include "HumanPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "DonkeyCartActor.h"

AHumanPlayerCharacter::AHumanPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	// Kamera boshqaruvida emas, harakat yo'nalishida aylanadi (uchinchi shaxs uslubi).
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Mannequin skeletal mesh вЂ” kapsulaga standart moslashtirilgan offset (Z=-88, Yaw=-90).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshFinder.Object);
	}
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -88.0f), FRotator(0.0f, -90.0f, 0.0f));

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	if (AnimBPFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBPFinder.Class);
	}
}

void AHumanPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AHumanPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHumanPlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHumanPlayerCharacter::TurnRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHumanPlayerCharacter::LookUpRate);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AHumanPlayerCharacter::Interact);
}

void AHumanPlayerCharacter::MoveForward(float Value)
{
	if (MountedCart)
	{
		MountedCart->SetDriveForwardInput(Value);
		return;
	}

	if (Controller && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void AHumanPlayerCharacter::MoveRight(float Value)
{
	if (MountedCart)
	{
		// Mount qilinganРґР° WSAD'РґР°gi A/D вЂ” o'zi yon yurish emas, aravani buradi.
		MountedCart->SetDriveTurnInput(Value);
		return;
	}

	if (Controller && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
	}
}

void AHumanPlayerCharacter::Interact()
{
	if (MountedCart)
	{
		ADonkeyCartActor* CartToLeave = MountedCart;
		MountedCart = nullptr;
		CartToLeave->DismountDriver();
		return;
	}

	TArray<AActor*> FoundCarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADonkeyCartActor::StaticClass(), FoundCarts);

	ADonkeyCartActor* NearestCart = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (AActor* CartActor : FoundCarts)
	{
		ADonkeyCartActor* Cart = Cast<ADonkeyCartActor>(CartActor);
		if (!Cart || Cart->IsMounted())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(GetActorLocation(), Cart->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestCart = Cart;
		}
	}

	if (NearestCart && NearestDistSq <= FMath::Square(NearestCart->MountRange))
	{
		NearestCart->MountDriver(this);
		MountedCart = NearestCart;
	}
}

void AHumanPlayerCharacter::TurnRate(float Value)
{
	AddControllerYawInput(Value * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHumanPlayerCharacter::LookUpRate(float Value)
{
	AddControllerPitchInput(Value * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

