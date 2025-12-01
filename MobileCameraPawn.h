#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MobileCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;

UCLASS()
class AMobileCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AMobileCameraPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UFloatingPawnMovement* MovementComp;

	// --- Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobile Touch")
	float PanSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobile Touch")
	float PanSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobile Touch")
	float InertiaDamping = 0.92f; // Lower = stops faster

	// --- State Variables ---
	bool bIsDragging = false;
	FVector2D LastTouchPos = FVector2D::ZeroVector;
	FVector2D TouchVelocity = FVector2D::ZeroVector; 
	FVector2D InertiaVelocity = FVector2D::ZeroVector;

	// --- Input Handlers ---
	void OnTouchPressed(const ETouchIndex::Type FingerIndex, const FVector Location);
	void OnTouchMoved(const ETouchIndex::Type FingerIndex, const FVector Location);
	void OnTouchReleased(const ETouchIndex::Type FingerIndex, const FVector Location);

	// --- Helper ---
	// Calculates world direction based on camera rotation
	FVector GetCameraPanDirection(FVector2D ScreenDelta) const;
};
