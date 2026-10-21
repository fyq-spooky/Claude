// Das Spielerfahrzeug: Fahrphysik (Bodenhaftung, Lenkung, Rampen, Spruenge), Wheelies,
// Crashes, Effekte und Sound. Elektrik und Laengsdynamik kommen aus dem Simulationskern.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ScooterSimCore.h"
#include "ScooterVehicleVisual.h"
#include "ScooterPawn.generated.h"

class UBoxComponent;
class USpringArmComponent;
class UCameraComponent;
class UMotorSynthComponent;
class AFireFx;
class AScooterPlayerController;

struct FScooterDriveInput
{
	float Throttle = 0.f;
	float Brake = 0.f;
	float Steer = 0.f;
	float Lean = 0.f; // +1 = zuruecklehnen
};

UCLASS()
class SCOOTERSIM_API AScooterPawn : public APawn
{
	GENERATED_BODY()

public:
	AScooterPawn();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetVelocity() const override;

	void SetProfile(ScooterSim::FProfile* InProfile);
	void RebuildVehicle();
	void SetDriveInput(const FScooterDriveInput& In) { Input = In; }
	void SetMenuFrozen(bool bFrozen) { bMenuFrozen = bFrozen; }
	void SetRaceFrozen(bool bFrozen) { bRaceFrozen = bFrozen; }
	void TeleportTo(const FVector& GroundLocation, float YawDeg);
	void ResetToLastSafe();
	void Crash(const FString& Reason);
	void ClearDamageFx();
	void SpawnFailureFx(ScooterSim::EFailure Failure);

	float GetSpeedKmh() const { return FMath::Abs(Ride.Speed) * 3.6f; }
	float GetSpeedMs() const { return Ride.Speed; }
	FVector GetForwardDir() const;
	const ScooterSim::FTelemetry& GetTelemetry() const { return Telemetry; }
	bool IsAirborne() const { return bAirborne; }
	bool IsCrashed() const { return bCrashed; }
	bool IsBroken() const;
	bool IsFrozen() const { return bMenuFrozen || bRaceFrozen; }
	bool IsInWheelie() const { return bInWheelie; }
	float GetWheelieDeg() const { return FMath::RadiansToDegrees(Ride.Pitch); }
	float GetBalanceDeg() const;
	float GetWheelieDistance() const { return WheelieDist; }
	float GetAirTime() const { return bAirborne ? AirTime : 0.f; }
	int32 GetVehicleIndex() const { return VehicleIndex; }
	float GetYaw() const { return Yaw; }

	static constexpr float BoxHalfHeightCm = 42.5f;
	static constexpr float BoxClearanceCm = 45.f;
	static constexpr float RootHeightCm = BoxClearanceCm + BoxHalfHeightCm;

	UPROPERTY(VisibleAnywhere, Category = "Scooter")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Scooter")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Scooter")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Scooter")
	TObjectPtr<UMotorSynthComponent> MotorSound;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> VisualParts;

	UPROPERTY()
	TArray<TObjectPtr<AFireFx>> DamageFx;

	UPROPERTY()
	TObjectPtr<AFireFx> SmokeFx;

private:
	AScooterPlayerController* GetScooterController() const;
	bool ProbeGround(const FVector& Point, float RefZ, float& OutZ) const;
	void StepCore(float Dt, bool bGrounded, const ScooterSim::FVehicleDef& Def, const ScooterSim::FDerived& D, float Throttle, float Brake, float Lean);
	void CoolDown(float Dt);
	void TickGrounded(float Dt, const ScooterSim::FVehicleDef& Def, const ScooterSim::FDerived& D);
	void TickAirborne(float Dt, const ScooterSim::FVehicleDef& Def, const ScooterSim::FDerived& D);
	void TakeOff(const FVector& Fwd);
	void Land(float Zf, float Zr, bool bHasFront, bool bHasRear, const FVector& Fwd, const ScooterSim::FVehicleDef& Def);
	bool HandleImpact(const FHitResult& Hit, const FVector& Fwd);
	void HandleEvents(float Dt);
	void EndWheelie(bool bPay);
	void UpdateSmoke();
	void UpdateVisuals(float Dt);
	void UpdateSound(const ScooterSim::FDerived& D);

	ScooterSim::FProfile* Profile = nullptr;
	int32 VehicleIndex = 0;
	FScooterVisual Visual;
	FScooterDriveInput Input;
	ScooterSim::FRideState Ride = ScooterSim::FRideState();
	ScooterSim::FTelemetry Telemetry = ScooterSim::FTelemetry();

	float Yaw = 0.f;
	bool bHasGround = false;
	bool bAirborne = false;
	FVector AirVelocity = FVector::ZeroVector; // m/s
	float FrontGroundZ = 0.f; // cm
	float RearGroundZ = 0.f;
	float SlopeRad = 0.f;
	float SteerAngleRad = 0.f;
	float LeanVisualDeg = 0.f;
	float LeanSmoothed = 0.f;
	float WheelAngleDeg = 0.f;
	float ReverseHold = 0.f;

	bool bMenuFrozen = false;
	bool bRaceFrozen = false;
	bool bCrashed = false;
	float CrashTimer = 0.f;

	FVector LastSafeLocation = FVector::ZeroVector;
	float LastSafeYaw = 0.f;
	float SafeTimer = 0.f;

	bool bInWheelie = false;
	float WheelieDist = 0.f;
	float WheelieTime = 0.f;
	float WheelieSpeedSum = 0.f;

	float AirTime = 0.f;
	float AirRotation = 0.f;

	float BmsNoticeCooldown = 0.f;
	float SmokeTimer = 0.f;
};
