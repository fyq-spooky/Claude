#include "PoliceManager.h"
#include "PoliceCar.h"
#include "ScooterWorld.h"
#include "ScooterPawn.h"
#include "ScooterPlayerController.h"
#include "ScooterShapes.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

APoliceManager::APoliceManager()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	Rng.Initialize(777);
}

void APoliceManager::BeginPlay()
{
	Super::BeginPlay();
	City = ScooterShapes::FindFirst<AScooterWorld>(GetWorld());
}

void APoliceManager::InitPatrols()
{
	// Erst im ersten Tick: dann laeuft die Welt schon und neue Autos bekommen sicher BeginPlay
	bInitialized = true;
	if (!City.IsValid())
	{
		City = ScooterShapes::FindFirst<AScooterWorld>(GetWorld());
	}
	if (!City.IsValid())
	{
		return;
	}
	for (const FVector& Cam : City->GetSpeedCameras())
	{
		CameraFlashes.Add(ScooterShapes::AddLight(this, SceneRoot, Cam + FVector(0.f, 0.f, 400.f), FLinearColor::White, 0.f, 3500.f));
	}
	for (int32 i = 0; i < NumPatrols; ++i)
	{
		SpawnCar(PickSpawnNode(City->GetSpawnLocation(), 200.f, 900.f), false);
	}
}

APoliceCar* APoliceManager::SpawnCar(int32 Node, bool bChasing)
{
	UWorld* World = GetWorld();
	if (!World || !City.IsValid())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APoliceCar* Car = World->SpawnActor<APoliceCar>(APoliceCar::StaticClass(), FTransform(City->GetNode(Node)), Params);
	if (Car)
	{
		Car->InitCar(City.Get(), this, Node, Rng.RandRange(1, 1000000));
		Car->SetChasing(bChasing, TopSpeedForHeat());
		Cars.Add(Car);
	}
	return Car;
}

int32 APoliceManager::PickSpawnNode(const FVector& PlayerLoc, float MinDistM, float MaxDistM)
{
	if (!City.IsValid() || City->NumNodes() == 0)
	{
		return 0;
	}
	int32 Fallback = 0;
	for (int32 Try = 0; Try < 40; ++Try)
	{
		const int32 Node = Rng.RandRange(0, City->NumNodes() - 1);
		const float D = (float)FVector::Dist2D(City->GetNode(Node), PlayerLoc) / 100.f;
		if (D >= MinDistM && D <= MaxDistM)
		{
			return Node;
		}
		if (D >= MinDistM)
		{
			Fallback = Node;
		}
	}
	return Fallback;
}

float APoliceManager::TopSpeedForHeat() const
{
	static const float Kmh[] = { 60.f, 95.f, 120.f, 150.f, 180.f, 215.f };
	return Kmh[FMath::Clamp(Heat, 0, 5)] / 3.6f;
}

float APoliceManager::GetEscapeProgress() const
{
	if (!bChase)
	{
		return 0.f;
	}
	return FMath::Clamp(NoSightTime / (6.f + 2.f * (float)Heat), 0.f, 1.f);
}

void APoliceManager::StartChase(const FString& Reason, int32 MinHeat)
{
	const bool bWasChasing = bChase;
	bChase = true;
	Heat = FMath::Max(Heat, MinHeat);
	if (!bWasChasing)
	{
		ChaseTime = 0.f;
		NoSightTime = 0.f;
		BustMeter = 0.f;
		SpawnTimer = 2.f;
	}
	for (APoliceCar* Car : Cars)
	{
		if (Car)
		{
			Car->SetChasing(true, TopSpeedForHeat());
		}
	}
	AScooterPawn* Pawn = Cast<AScooterPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (AScooterPlayerController* PC = Pawn ? Cast<AScooterPlayerController>(Pawn->GetController()) : nullptr)
	{
		PC->Notify(Reason + TEXT(" Hau ab!"), FLinearColor(1.f, 0.2f, 0.2f), 3.f, true);
	}
}

void APoliceManager::EndChase(bool bResetCars)
{
	bChase = false;
	bSeen = false;
	Heat = 0;
	BustMeter = 0.f;
	NoSightTime = 0.f;
	CooldownAfterChase = 8.f;
	for (int32 i = Cars.Num() - 1; i >= 0; --i)
	{
		APoliceCar* Car = Cars[i];
		if (!Car)
		{
			Cars.RemoveAt(i);
			continue;
		}
		if (i >= NumPatrols)
		{
			Car->Destroy();
			Cars.RemoveAt(i);
			continue;
		}
		Car->SetChasing(false, 0.f);
	}
	if (bResetCars && City.IsValid())
	{
		const FVector Spawn = City->GetSpawnLocation();
		for (APoliceCar* Car : Cars)
		{
			if (Car)
			{
				Car->PlaceAtNode(PickSpawnNode(Spawn, 300.f, 900.f));
			}
		}
	}
}

void APoliceManager::Escaped()
{
	AScooterPawn* Pawn = Cast<AScooterPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (AScooterPlayerController* PC = Pawn ? Cast<AScooterPlayerController>(Pawn->GetController()) : nullptr)
	{
		const float H = (float)Heat;
		PC->GetProfile().Escapes++;
		PC->AddMoney(100.f * H + 60.f * H * H, FString::Printf(TEXT("POLIZEI ABGEH\u00c4NGT! (Fahndungslevel %d)"), Heat));
	}
	EndChase(false);
}

void APoliceManager::Busted()
{
	const float Fine = 100.f + 150.f * (float)Heat;
	EndChase(true);
	AScooterPawn* Pawn = Cast<AScooterPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (AScooterPlayerController* PC = Pawn ? Cast<AScooterPlayerController>(Pawn->GetController()) : nullptr)
	{
		PC->OnBusted(Fine);
	}
}

void APoliceManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInitialized)
	{
		InitPatrols();
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	AScooterPawn* Pawn = Cast<AScooterPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Pawn || !City.IsValid())
	{
		return;
	}
	AScooterPlayerController* PC = Cast<AScooterPlayerController>(Pawn->GetController());
	const FVector PLoc = Pawn->GetActorLocation();
	const float Kmh = Pawn->GetSpeedKmh();
	CameraCooldown -= Dt;
	FlashTimer -= Dt;
	CooldownAfterChase -= Dt;
	Cars.RemoveAll([](const TObjectPtr<APoliceCar>& Car) { return !IsValid(Car); });

	// Blitzer
	const TArray<FVector>& Cams = City->GetSpeedCameras();
	for (int32 i = 0; i < Cams.Num(); ++i)
	{
		UPointLightComponent* Flash = CameraFlashes.IsValidIndex(i) ? CameraFlashes[i].Get() : nullptr;
		const bool bNear = FVector::Dist2D(PLoc, Cams[i]) < 1300.0;
		if (bNear && Kmh > 50.f && CameraCooldown <= 0.f && !Pawn->IsFrozen())
		{
			CameraCooldown = 4.f;
			FlashTimer = 0.18f;
			if (Flash)
			{
				Flash->SetIntensity(60000.f);
			}
			if (PC)
			{
				PC->Notify(FString::Printf(TEXT("GEBLITZT! %.0f km/h"), Kmh), FLinearColor(1.f, 1.f, 1.f), 2.5f, true);
			}
			if (!bChase)
			{
				StartChase(TEXT("Der Blitzer hat die Polizei gerufen!"), 1);
			}
			else
			{
				Heat = FMath::Min(5, Heat + 1);
			}
		}
		if (Flash && FlashTimer <= 0.f)
		{
			Flash->SetIntensity(0.f);
		}
	}

	if (!bChase)
	{
		bSeen = false;
		if (CooldownAfterChase <= 0.f && Kmh > LegalKmh && !Pawn->IsFrozen())
		{
			for (APoliceCar* Car : Cars)
			{
				if (Car && FVector::Dist(Car->GetActorLocation(), PLoc) < 8000.0 && Car->HasLineOfSightTo(PLoc))
				{
					StartChase(FString::Printf(TEXT("Die Polizei hat dich mit %.0f km/h gesehen!"), Kmh), 1);
					break;
				}
			}
		}
	}
	else
	{
		ChaseTime += Dt;
		const int32 SpeedHeat = Kmh > 150.f ? 4 : (Kmh > 110.f ? 3 : (Kmh > 75.f ? 2 : 1));
		const int32 TimeHeat = 1 + (int32)(ChaseTime / 45.f);
		const int32 NewHeat = FMath::Clamp(FMath::Max(Heat, FMath::Max(SpeedHeat, TimeHeat)), 1, 5);
		if (NewHeat > Heat && PC)
		{
			PC->Notify(FString::Printf(TEXT("Fahndungslevel %d!"), NewHeat), FLinearColor(1.f, 0.3f, 0.3f), 2.f, false);
		}
		Heat = NewHeat;

		bSeen = false;
		bool bNearCar = false;
		for (APoliceCar* Car : Cars)
		{
			if (!Car)
			{
				continue;
			}
			const float D = (float)FVector::Dist2D(Car->GetActorLocation(), PLoc) / 100.f;
			if (D < 8.f)
			{
				bNearCar = true;
			}
			if (D < 25.f || (D < 140.f && Car->HasLineOfSightTo(PLoc)))
			{
				bSeen = true;
			}
		}
		NoSightTime = bSeen ? 0.f : NoSightTime + Dt;
		if (NoSightTime > 6.f + 2.f * (float)Heat)
		{
			Escaped();
			return;
		}

		const bool bHelpless = Kmh < 12.f || Pawn->IsCrashed() || Pawn->IsBroken();
		if (bNearCar && bHelpless && !Pawn->IsFrozen())
		{
			BustMeter += Dt / 2.5f;
		}
		else
		{
			BustMeter = FMath::Max(0.f, BustMeter - Dt / 3.f);
		}
		if (BustMeter >= 1.f)
		{
			Busted();
			return;
		}

		SpawnTimer -= Dt;
		if (Cars.Num() < NumPatrols + Heat && SpawnTimer <= 0.f)
		{
			SpawnTimer = 5.f;
			SpawnCar(PickSpawnNode(PLoc, 150.f, 350.f), true);
		}
		const float Top = TopSpeedForHeat();
		for (APoliceCar* Car : Cars)
		{
			if (Car)
			{
				Car->SetChasing(true, Top);
			}
		}
	}

	for (APoliceCar* Car : Cars)
	{
		if (Car)
		{
			const float D = (float)FVector::Dist(Car->GetActorLocation(), PLoc) / 100.f;
			Car->SetSirenVolume(Car->IsChasing() ? FMath::Clamp(1.f - D / 220.f, 0.f, 1.f) * 0.6f : 0.f);
		}
	}
}
