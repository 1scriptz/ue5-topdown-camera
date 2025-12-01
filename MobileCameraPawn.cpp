#include "MobileCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"

AMobileCameraPawn::AMobileCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. Root (No Collision)
    USphereComponent* SphereRoot = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
    SphereRoot->SetSphereRadius(10.0f);
    SphereRoot->SetCollisionProfileName(TEXT("NoCollision")); 
    RootComponent = SphereRoot;

    // 2. Spring Arm
    SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
    SpringArmComp->SetupAttachment(RootComponent);
    SpringArmComp->TargetArmLength = 1500.0f; // Standard RTS distance
    SpringArmComp->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f)); // -60 is a good "Top Down" angle
    SpringArmComp->bDoCollisionTest = false; 
    SpringArmComp->bUsePawnControlRotation = false; // Don't let controller rotate arm
    SpringArmComp->bInheritYaw = true;

    // 3. Camera
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
    CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
    CameraComp->ProjectionMode = ECameraProjectionMode::Perspective; // Changed from Ortho to Perspective
    CameraComp->FieldOfView = 90.0f;

    // 4. Movement
    MovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
    MovementComp->MaxSpeed = 0.0f;
    MovementComp->Deceleration = 0.0f;
}

void AMobileCameraPawn::BeginPlay()
{
    Super::BeginPlay();

    // Ensure mouse cursor is visible so you don't feel "consumed"
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;
    }
}

void AMobileCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindTouch(IE_Pressed, this, &AMobileCameraPawn::OnTouchPressed);
    PlayerInputComponent->BindTouch(IE_Repeat, this, &AMobileCameraPawn::OnTouchMoved);
    PlayerInputComponent->BindTouch(IE_Released, this, &AMobileCameraPawn::OnTouchReleased);
}

void AMobileCameraPawn::OnTouchPressed(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (FingerIndex != ETouchIndex::Touch1) return;

    bIsDragging = true;
    LastTouchPos = FVector2D(Location.X, Location.Y);
    
    // Reset Inertia
    TouchVelocity = FVector2D::ZeroVector;
    InertiaVelocity = FVector2D::ZeroVector;

    UKismetSystemLibrary::PrintString(this, TEXT("PRESSED"), true, true, FLinearColor::Green, 1.0f);
}

void AMobileCameraPawn::OnTouchMoved(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (FingerIndex != ETouchIndex::Touch1 || !bIsDragging) return;

    FVector2D CurrentPos = FVector2D(Location.X, Location.Y);
    float DT = GetWorld()->GetDeltaSeconds();
    if (DT <= 0.0001f) return;

    // 1. Calculate Screen Delta
    FVector2D ScreenDelta = CurrentPos - LastTouchPos;

    // 2. Calculate Velocity (Pixels/Sec)
    TouchVelocity = (ScreenDelta / DT) * PanSensitivity;

    // 3. Convert Screen Movement to World Movement (Relative to Camera Rotation)
    FVector WorldMove = GetCameraPanDirection(ScreenDelta);

    // 4. Apply
    AddActorWorldOffset(WorldMove, false); // bSweep=false to prevent sticking

    LastTouchPos = CurrentPos;

    // Debug
    FString LogMsg = FString::Printf(TEXT("MOVED: X=%.1f Y=%.1f"), ScreenDelta.X, ScreenDelta.Y);
    UKismetSystemLibrary::PrintString(this, LogMsg, true, true, FLinearColor::Blue, 0.0f);
}

void AMobileCameraPawn::OnTouchReleased(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (FingerIndex != ETouchIndex::Touch1) return;

    if (bIsDragging)
    {
        bIsDragging = false;
        InertiaVelocity = TouchVelocity;
        UKismetSystemLibrary::PrintString(this, TEXT("RELEASED"), true, true, FLinearColor::Red, 2.0f);
    }
}

void AMobileCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsDragging)
    {
        if (InertiaVelocity.SizeSquared() > 100.0f)
        {
            // Apply Friction
            InertiaVelocity *= InertiaDamping;

            // Calculate movement based on Inertia Velocity
            // We simulate a "fake" screen delta based on velocity * time
            FVector2D FakeScreenDelta = InertiaVelocity * DeltaTime;

            // Convert to World Space
            FVector WorldMove = GetCameraPanDirection(FakeScreenDelta);

            AddActorWorldOffset(WorldMove, false);
        }
        else
        {
            InertiaVelocity = FVector2D::ZeroVector;
        }
    }
}

FVector AMobileCameraPawn::GetCameraPanDirection(FVector2D ScreenDelta) const
{
    // 1. Get Camera Rotation (Yaw Only)
    FRotator CamRot = CameraComp->GetComponentRotation();
    CamRot.Pitch = 0.0f;
    CamRot.Roll = 0.0f;

    // 2. Get Forward and Right Vectors aligned to the ground
    FVector CamForward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
    FVector CamRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);

    // 3. Calculate Direction
    // Dragging RIGHT (+X) should move camera LEFT (-RightVector) to keep world under finger
    // Dragging DOWN (+Y) should move camera UP (+ForwardVector)
    
    FVector Direction = (CamRight * -ScreenDelta.X) + (CamForward * ScreenDelta.Y);
    
    return Direction * PanSpeed;
}
