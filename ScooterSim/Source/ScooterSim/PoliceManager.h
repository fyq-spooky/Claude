// Polizei-Logik: Streifen, Blitzer, Verfolgung, Fahndungslevel, Entkommen und Geschnappt-werden.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoliceManager.generated.h"

class APoliceCar;
class AScooterWorld;
class AScooterPawn;
class AScooterPlayerController;
class UPointLightComponent;

UCLASS()
class SCOOTERSIM_API APoliceManager : public AActor
{
	GENERATED_BODY()

public:
	APoliceManager();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool IsChasing() const { return bChase; }
	int32 GetHeat() const { return Heat; }
	float GetBustProgress() const { return BustMeter; }
	float GetEscapeProgress() const;
	bool IsPlayerSeen() const { return bSeen; }
	const TArray<TObjectPtr<APoliceCar>>& GetCars() const { return Cars; }
	void EndChase(bool bResetCars);

	// Legales Tempo fuer E-Scooter/Pedelecs (mit etwas Toleranz)
	static constexpr float LegalKmh = 30.f;

protected:
	UPROPERTY()
	TArray<TObjectPtr<APoliceCar>> Cars;

	UPROPERTY()
	TArray<TObjectPtr<UPointLightComponent>> CameraFlashes;

	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	void InitPatrols();
	APoliceCar* SpawnCar(int32 Node, bool bChasing);
	int32 PickSpawnNode(const FVector& PlayerLoc, float MinDistM, float MaxDistM);
	void StartChase(const FString& Reason, int32 MinHeat);
	void Escaped();
	void Busted();
	float TopSpeedForHeat() const;

	TWeakObjectPtr<AScooterWorld> City;
	FRandomStream Rng;
	bool bInitialized = false;
	bool bChase = false;
	bool bSeen = false;
	int32 Heat = 0;
	int32 NumPatrols = 3;
	float ChaseTime = 0.f;
	float NoSightTime = 0.f;
	float BustMeter = 0.f;
	float SpawnTimer = 0.f;
	float CameraCooldown = 0.f;
	float FlashTimer = 0.f;
	float CooldownAfterChase = 0.f;
};
