// Erzeugt die komplette Spielwelt zur Laufzeit: Licht/Himmel, Stadt, Parks, Skatepark,
// Highway, Garage. Stellt ausserdem den Strassengraphen fuer Polizei und Gegner bereit.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScooterShapes.h"
#include "ScooterWorld.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;

UCLASS()
class SCOOTERSIM_API AScooterWorld : public AActor
{
	GENERATED_BODY()

public:
	AScooterWorld();
	virtual void BeginPlay() override;

	// Baut die Welt (wird vom GameMode direkt nach dem Spawnen aufgerufen).
	void Build();

	// Layout in Metern
	static constexpr int32 GridCount = 9;
	static constexpr float BlockM = 110.f;
	static constexpr float RoadM = 16.f;
	static constexpr float SidewalkM = 4.f;
	static constexpr float HighwayLengthM = 2600.f;
	static constexpr float HighwayWidthM = 24.f;

	FVector GridPoint(int32 I, int32 J) const;
	FVector HighwayPoint(float DistM) const;
	FVector GetSpawnLocation() const;
	float GetSpawnYaw() const { return 0.f; }
	bool IsInGarage(const FVector& P) const;
	bool IsInPark(const FVector& P) const;
	FBox2D GetMapBounds() const;
	FVector GetRadarGate() const;

	int32 NumNodes() const { return NodePos.Num(); }
	FVector GetNode(int32 Index) const;
	const TArray<int32>& GetLinks(int32 Index) const;
	int32 GridNode(int32 I, int32 J) const { return I * GridCount + J; }
	int32 FindNearestNode(const FVector& P) const;
	int32 NextNodeToward(int32 From, int32 Goal) const;
	int32 RandomNeighbor(int32 Node, int32 Avoid, FRandomStream& Rng) const;
	const TArray<FBox2D>& GetParks() const { return ParkZones; }
	const TArray<FVector>& GetSpeedCameras() const { return SpeedCameras; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "World")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "World")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "World")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "World")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "World")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Instanced;

private:
	UInstancedStaticMeshComponent* MakeIsm(EScooterShape Shape, uint32 Color, bool bCollision, bool bShadow = true);
	void AddBox(UInstancedStaticMeshComponent* Target, const FVector& CenterM, const FVector& SizeM, const FRotator& Rot = FRotator::ZeroRotator);
	void AddRamp(UInstancedStaticMeshComponent* Target, const FVector& StartM, float YawDeg, float LengthM, float HeightM, float WidthM);
	void AddBollardLine(const FVector& FromM, const FVector& ToM, float SpacingM);
	void AddLabel(const FString& Text, const FVector& LocationM, float YawDeg, float SizeM, const FColor& Color);

	void BuildGround();
	void BuildRoads();
	void BuildBlocks();
	void BuildPark(const FBox2D& Area, FRandomStream& Rng);
	void BuildSkatepark(const FBox2D& Area);
	void BuildBuildings(const FBox2D& Area, float CenterFactor, FRandomStream& Rng);
	void BuildGarage();
	void BuildHighway();
	void BuildScenery();
	void BuildGraph();
	void Link(int32 A, int32 B);

	bool bBuilt = false;
	TArray<FVector> NodePos;
	TArray<TArray<int32>> NodeLinks;
	TArray<FBox2D> ParkZones;
	TArray<FVector> SpeedCameras;
	FBox2D GarageZone;

	UInstancedStaticMeshComponent* IsmGround = nullptr;
	UInstancedStaticMeshComponent* IsmAsphalt = nullptr;
	UInstancedStaticMeshComponent* IsmMarking = nullptr;
	UInstancedStaticMeshComponent* IsmSidewalk = nullptr;
	UInstancedStaticMeshComponent* IsmGrass = nullptr;
	UInstancedStaticMeshComponent* IsmPath = nullptr;
	UInstancedStaticMeshComponent* IsmWindows = nullptr;
	UInstancedStaticMeshComponent* IsmRoof = nullptr;
	UInstancedStaticMeshComponent* IsmTrunk = nullptr;
	UInstancedStaticMeshComponent* IsmCrown = nullptr;
	UInstancedStaticMeshComponent* IsmBollard = nullptr;
	UInstancedStaticMeshComponent* IsmRamp = nullptr;
	UInstancedStaticMeshComponent* IsmConcrete = nullptr;
	UInstancedStaticMeshComponent* IsmBarrier = nullptr;
	UInstancedStaticMeshComponent* IsmGarage = nullptr;
	UInstancedStaticMeshComponent* IsmMetal = nullptr;
	UInstancedStaticMeshComponent* IsmFountain = nullptr;
	TArray<UInstancedStaticMeshComponent*> IsmBuildings;
};
