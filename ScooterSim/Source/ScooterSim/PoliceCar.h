// Streifenwagen: faehrt Streife ueber den Strassengraphen oder verfolgt den Spieler.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoliceCar.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMotorSynthComponent;
class AScooterWorld;
class APoliceManager;

UCLASS()
class SCOOTERSIM_API APoliceCar : public AActor
{
	GENERATED_BODY()

public:
	APoliceCar();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void InitCar(AScooterWorld* InWorld, APoliceManager* InManager, int32 StartNode, int32 Seed);
	void SetChasing(bool bInChasing, float InTopSpeedMs);
	void SetSirenVolume(float Volume);
	void PlaceAtNode(int32 Node);
	bool IsChasing() const { return bChasing; }
	float GetSpeed() const { return Speed; }
	bool HasLineOfSightTo(const FVector& Target) const;

	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<UMotorSynthComponent> Siren;

protected:
	UPROPERTY()
	TObjectPtr<UPointLightComponent> BeaconLightA;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> BeaconLightB;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BeaconA;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BeaconB;

private:
	void BuildBody();
	FVector ChooseTarget(float& OutDesiredSpeed);

	TWeakObjectPtr<AScooterWorld> City;
	TWeakObjectPtr<APoliceManager> Manager;
	FRandomStream Rng;
	int32 TargetNode = 0;
	int32 PrevNode = 0;
	bool bBodyBuilt = false;
	bool bChasing = false;
	float Speed = 0.f;
	float Yaw = 0.f;
	float TopSpeed = 11.f;
	float StuckTimer = 0.f;
	float ReverseTimer = 0.f;
	float FlashTime = 0.f;
	float RamCooldown = 0.f;
	float WheelAngle = 0.f;
	TArray<USceneComponent*> Wheels;
};
