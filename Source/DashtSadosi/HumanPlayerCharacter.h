#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HumanPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ADonkeyCartActor;

/**
 * Odamga o'xshagan o'ynaladigan personaj (Mannequin skeletal mesh + AnimBP orqali
 * yurish/yugurish/sakrash animatsiyalari). Harakat вЂ” standart UCharacterMovementComponent,
 * kirish вЂ” klassik Axis/Action mapping (Config/DefaultInput.ini): MoveForward, MoveRight,
 * Turn, LookUp, Jump, Interact (E вЂ” ot-aravaga mount/dismount).
 * Mount qilinganРґР° WSAD o'zining harakati o'rniga ADonkeyCartActor'ni haydaydi.
 */
UCLASS()
class DASHTSADOSI_API AHumanPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHumanPlayerCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Human|Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Human|Camera")
	UCameraComponent* FollowCamera;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnRate(float Value);
	void LookUpRate(float Value);
	void Interact();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Human|Camera", meta = (ClampMin = "0.0"))
	float BaseTurnRate = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Human|Camera", meta = (ClampMin = "0.0"))
	float BaseLookUpRate = 90.0f;

	// Hozir mount qilingan ot-arava (yo'q bo'lsa nullptr). WSAD shu bo'lsa uni haydaydi.
	UPROPERTY(BlueprintReadOnly, Category = "Human|Drive")
	ADonkeyCartActor* MountedCart = nullptr;
};

