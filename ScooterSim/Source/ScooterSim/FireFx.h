// Feuer- und Rauch-Effekt aus flackernden Grundformen + Licht (ohne Partikel-Assets).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireFx.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class SCOOTERSIM_API AFireFx : public AActor
{
	GENERATED_BODY()

public:
	AFireFx();
	virtual void Tick(float DeltaSeconds) override;

	// Scale: 1 = kleines Kabelfeuer, 3 = Akkubrand. bSmokeOnly: nur Rauch (heisses Kabel).
	void Setup(float InScale, bool bInSmokeOnly, float LifeSeconds);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Fx")
	TObjectPtr<USceneComponent> FxRoot;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Flames;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Puffs;

private:
	float FxScale = 1.f;
	bool bSmokeOnly = false;
	float Life = 0.f;
	float Age = 0.f;
	TArray<float> PuffAge;
};
