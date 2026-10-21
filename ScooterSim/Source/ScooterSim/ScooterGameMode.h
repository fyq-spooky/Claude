// Startet das Spiel: baut die Welt, spawnt Polizei und Rennen, setzt Pawn/Controller/HUD.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ScooterGameMode.generated.h"

UCLASS()
class SCOOTERSIM_API AScooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AScooterGameMode();
	virtual void StartPlay() override;
};
