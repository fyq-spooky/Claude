#include "FireFx.h"
#include "ScooterShapes.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

AFireFx::AFireFx()
{
	PrimaryActorTick.bCanEverTick = true;
	FxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	FxRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(FxRoot);
}

void AFireFx::Setup(float InScale, bool bInSmokeOnly, float LifeSeconds)
{
	FxScale = FMath::Max(InScale, 0.2f);
	bSmokeOnly = bInSmokeOnly;
	Life = LifeSeconds;
	Age = 0.f;

	if (!bSmokeOnly)
	{
		const uint32 Colors[] = { 0xFF5A00, 0xFFB000, 0xFF2A00, 0xFFD54A, 0xFF7A00, 0xE02000 };
		for (int32 i = 0; i < 6; ++i)
		{
			const float S = 12.f * FxScale;
			UStaticMeshComponent* Flame = ScooterShapes::AddShape(this, FxRoot, i % 2 == 0 ? EScooterShape::Cone : EScooterShape::Sphere,
				FVector::ZeroVector, FVector(S, S, S * 1.6f), ScooterShapes::Hex(Colors[i]));
			Flame->SetCastShadow(false);
			Flames.Add(Flame);
		}
		Glow = ScooterShapes::AddLight(this, FxRoot, FVector(0.f, 0.f, 20.f * FxScale), FLinearColor(1.f, 0.45f, 0.1f), 1500.f * FxScale, 900.f * FxScale);
	}
	const int32 NumPuffs = bSmokeOnly ? 5 : 8;
	for (int32 i = 0; i < NumPuffs; ++i)
	{
		UStaticMeshComponent* Puff = ScooterShapes::AddShape(this, FxRoot, EScooterShape::Sphere, FVector::ZeroVector,
			FVector(10.f, 10.f, 10.f), ScooterShapes::Hex(bSmokeOnly ? 0x8A8A8A : 0x2A2A2A));
		Puff->SetCastShadow(false);
		Puffs.Add(Puff);
		PuffAge.Add((float)i / (float)NumPuffs);
	}
}

void AFireFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Life > 0.f && Age > Life)
	{
		Destroy();
		return;
	}
	const float T = Age;
	for (int32 i = 0; i < Flames.Num(); ++i)
	{
		UStaticMeshComponent* Flame = Flames[i];
		if (!Flame)
		{
			continue;
		}
		const float Fi = (float)i;
		const float Flicker = 0.7f + 0.3f * FMath::Sin(T * (13.f + Fi * 3.1f) + Fi * 1.7f);
		const float S = 0.12f * FxScale * Flicker * (1.f - 0.1f * Fi);
		Flame->SetRelativeLocation(FVector(FMath::Sin(T * 7.f + Fi) * 4.f * FxScale, FMath::Cos(T * 9.f + Fi * 2.f) * 4.f * FxScale, (8.f + Fi * 5.f) * FxScale * Flicker));
		Flame->SetRelativeScale3D(FVector(S, S, S * (1.4f + 0.5f * Flicker)));
	}
	if (Glow)
	{
		Glow->SetIntensity((1100.f + 500.f * FMath::Sin(T * 23.f) + 300.f * FMath::Sin(T * 37.f)) * FxScale);
	}
	const float Rise = bSmokeOnly ? 0.6f : 0.45f;
	for (int32 i = 0; i < Puffs.Num(); ++i)
	{
		UStaticMeshComponent* Puff = Puffs[i];
		if (!Puff)
		{
			continue;
		}
		PuffAge[i] += DeltaSeconds * Rise;
		if (PuffAge[i] > 1.f)
		{
			PuffAge[i] -= 1.f;
		}
		const float A = PuffAge[i];
		const float Size = (0.1f + A * 0.6f) * FxScale * (bSmokeOnly ? 0.6f : 1.f);
		Puff->SetRelativeLocation(FVector(FMath::Sin((float)i * 2.3f + T) * 15.f * A, FMath::Cos((float)i * 1.9f + T) * 15.f * A, (20.f + A * 220.f) * FxScale));
		Puff->SetRelativeScale3D(FVector(Size, Size, Size) * (A < 0.9f ? 1.f : (1.f - A) * 10.f));
	}
}
