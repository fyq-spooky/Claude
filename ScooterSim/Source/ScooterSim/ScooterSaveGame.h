// Spielstand: Geld, Fahrzeuge, verbaute Teile, Einstellungen, Rekorde.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ScooterSimCore.h"
#include "ScooterSaveGame.generated.h"

UCLASS()
class SCOOTERSIM_API UScooterSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 Version = 0;

	UPROPERTY()
	int32 NumVehicles = 0;

	UPROPERTY()
	float Money = 0.f;

	UPROPERTY()
	int32 Current = 0;

	UPROPERTY()
	TArray<int32> Ints;

	UPROPERTY()
	TArray<float> Floats;

	UPROPERTY()
	int32 RacesWon = 0;

	UPROPERTY()
	int32 RadarMask = 0;

	UPROPERTY()
	int32 Escapes = 0;

	UPROPERTY()
	int32 Busts = 0;

	UPROPERTY()
	float BestTopSpeed = 0.f;

	UPROPERTY()
	float BestWheelie = 0.f;

	UPROPERTY()
	float TotalWheelie = 0.f;

	void FromProfile(const ScooterSim::FProfile& P);
	bool ToProfile(ScooterSim::FProfile& P) const;
};
