#include "ScooterGameMode.h"
#include "ScooterPawn.h"
#include "ScooterPlayerController.h"
#include "ScooterHUD.h"
#include "ScooterWorld.h"
#include "ScooterShapes.h"
#include "PoliceManager.h"
#include "RaceManager.h"
#include "Engine/World.h"

AScooterGameMode::AScooterGameMode()
{
	DefaultPawnClass = AScooterPawn::StaticClass();
	PlayerControllerClass = AScooterPlayerController::StaticClass();
	HUDClass = AScooterHUD::StaticClass();
}

void AScooterGameMode::StartPlay()
{
	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AScooterWorld* City = ScooterShapes::FindFirst<AScooterWorld>(World);
		if (!City)
		{
			City = World->SpawnActor<AScooterWorld>(AScooterWorld::StaticClass(), FTransform::Identity, Params);
		}
		if (City)
		{
			City->Build();
		}
		if (!ScooterShapes::FindFirst<APoliceManager>(World))
		{
			World->SpawnActor<APoliceManager>(APoliceManager::StaticClass(), FTransform::Identity, Params);
		}
		if (!ScooterShapes::FindFirst<ARaceManager>(World))
		{
			World->SpawnActor<ARaceManager>(ARaceManager::StaticClass(), FTransform::Identity, Params);
		}
	}
	Super::StartPlay();
}
