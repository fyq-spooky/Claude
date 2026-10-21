// Rennfahrer-KI: faehrt eine Wegpunkt-Liste ab. Beschleunigung und Topspeed kommen aus
// derselben Elektrik-Simulation wie beim Spieler, die Gegner fahren also "echte" Fahrzeuge.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScooterSimCore.h"
#include "ScooterVehicleVisual.h"
#include "RacerBot.generated.h"

UCLASS()
class SCOOTERSIM_API ARacerBot : public AActor
{
	GENERATED_BODY()

public:
	ARacerBot();
	virtual void Tick(float DeltaSeconds) override;

	void Setup(int32 InVehicleIndex, const ScooterSim::FVehicleSetup& InSetup, const TArray<FVector>& InPath, float InSkill,
		const FLinearColor& Jacket, const FLinearColor& Helmet, const FVector& StartLocation, float StartYaw);
	void Go() { bRunning = true; }
	bool HasFinished() const { return bFinished; }
	int32 GetNextIndex() const { return NextIndex; }
	float GetSpeedKmh() const { return FMath::Abs(Ride.Speed) * 3.6f; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Racer")
	TObjectPtr<USceneComponent> BotRoot;

	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> VisualParts;

private:
	FScooterVisual Visual;
	int32 VehicleIndex = 0;
	ScooterSim::FVehicleSetup BotSetup;
	ScooterSim::FVehicleState BotState;
	ScooterSim::FRideState Ride = ScooterSim::FRideState();
	TArray<FVector> Path;
	int32 NextIndex = 0;
	float Skill = 1.f;
	float Yaw = 0.f;
	float LeanDeg = 0.f;
	float WheelAngle = 0.f;
	bool bRunning = false;
	bool bFinished = false;
};
