// Rennen, Wheelie-Challenges und die Topspeed-Radarfalle.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RaceManager.generated.h"

class ARacerBot;
class AScooterWorld;
class AScooterPawn;
class AScooterPlayerController;
class UStaticMeshComponent;
class UPointLightComponent;

enum class EScooterRaceState : uint8
{
	Idle,
	Countdown,
	Running,
	Result
};

struct FRaceHudInfo
{
	bool bActive = false;
	FString Title;
	FString Line1;
	FString Line2;
	float Countdown = 0.f;
	bool bHasTarget = false;
	FVector Target = FVector::ZeroVector;
};

UCLASS()
class SCOOTERSIM_API ARaceManager : public AActor
{
	GENERATED_BODY()

public:
	ARaceManager();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool GetPrompt(const FVector& PlayerLoc, FString& OutText) const;
	bool TryStartNearby(AScooterPlayerController* PC);
	bool IsRaceActive() const { return State != EScooterRaceState::Idle; }
	void AbortRace(const FString& Reason);
	FRaceHudInfo GetHudInfo() const;
	int32 NumRaces() const;
	FVector GetMarker(int32 Index) const;
	bool IsRaceWon(int32 Index) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Race")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<ARacerBot> Bot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> CheckpointMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> NextCheckpointMesh;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> CheckpointLight;

private:
	void BuildPath(int32 RaceIndex, TArray<FVector>& OutPath) const;
	void StartRace(int32 RaceIndex, AScooterPlayerController* PC);
	void FinishRace(bool bWon, const FString& Text);
	void UpdateCheckpoints();
	void TickRadar(AScooterPawn* Pawn, AScooterPlayerController* PC, float Dt);
	AScooterPawn* GetPlayer() const;
	AScooterPlayerController* GetPC() const;

	TWeakObjectPtr<AScooterWorld> City;
	TArray<FVector> Markers;
	TArray<FVector> Path;
	EScooterRaceState State = EScooterRaceState::Idle;
	int32 ActiveRace = -1;
	int32 PlayerNext = 0;
	float Timer = 0.f;
	float RaceTime = 0.f;
	FString ResultText;
	bool bResultWon = false;
	float RadarCooldown = 0.f;
	float PrevPlayerX = 0.f;
};
