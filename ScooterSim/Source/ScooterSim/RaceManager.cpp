#include "RaceManager.h"
#include "RacerBot.h"
#include "ScooterWorld.h"
#include "ScooterPawn.h"
#include "ScooterPlayerController.h"
#include "ScooterShapes.h"
#include "ScooterSimCore.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include <initializer_list>

namespace SS = ScooterSim;

namespace
{
	enum class ERmKind : uint8 { Circuit, Drag, Wheelie };

	struct FRmRaceDef
	{
		const TCHAR* Name;
		ERmKind Kind;
		const TCHAR* Opponent;
		int32 OpponentVehicle;
		float Skill;
		float Stake;
		float Prize;
		float Param; // Wheelie: Distanz in m
		uint32 JacketColor;
	};

	const FRmRaceDef RmRaces[] = {
		{ TEXT("Kumpel-Duell (Drag 200 m)"),      ERmKind::Drag,    TEXT("Tim"),       1,  1.0f,  20.f,   70.f,   0.f,   0x2F7D32 },
		{ TEXT("Block-Runde (2 Runden)"),         ERmKind::Circuit, TEXT("Mia"),       3,  0.95f, 50.f,   180.f,  0.f,   0xAD1457 },
		{ TEXT("Highway-Drag 400 m"),             ERmKind::Drag,    TEXT("Kevin"),     7,  1.0f,  120.f,  450.f,  0.f,   0xC62828 },
		{ TEXT("Innenstadt-Sprint"),              ERmKind::Circuit, TEXT("Lena"),      8,  0.97f, 200.f,  750.f,  0.f,   0x1565C0 },
		{ TEXT("Dualtron-Treffen (1 km Drag)"),   ERmKind::Drag,    TEXT("Dragan"),    9,  1.0f,  400.f,  1500.f, 0.f,   0xF9A825 },
		{ TEXT("Sur-Ron-Gang Stadtkurs"),         ERmKind::Circuit, TEXT("Ronny"),     11, 1.0f,  600.f,  2400.f, 0.f,   0x558B2F },
		{ TEXT("Highway-K\u00f6nig (2 km Drag)"), ERmKind::Drag,    TEXT("Der K\u00f6nig"), 15, 1.0f, 1500.f, 8000.f, 0.f, 0x6A1B9A },
		{ TEXT("Wheelie-Challenge 50 m"),         ERmKind::Wheelie, TEXT(""),          0,  1.0f,  0.f,    150.f,  50.f,  0 },
		{ TEXT("Wheelie-Challenge 200 m"),        ERmKind::Wheelie, TEXT(""),          0,  1.0f,  0.f,    700.f,  200.f, 0 },
	};
	constexpr int32 RmNumRaces = (int32)(sizeof(RmRaces) / sizeof(RmRaces[0]));

	const float RmRadarKmh[] = { 60.f, 100.f, 140.f, 180.f, 220.f };
	const float RmRadarPrize[] = { 100.f, 300.f, 800.f, 2000.f, 5000.f };

	SS::FVehicleSetup RmOpponentSetup(int32 RaceIndex)
	{
		const FRmRaceDef& R = RmRaces[RaceIndex];
		SS::FVehicleSetup S = SS::MakeStockSetup(R.OpponentVehicle);
		S.bSpeedLimit = false;
		if (RaceIndex == 6)
		{
			// Der Koenig: voll getunter Hyperscooter
			S.Parts[(int32)SS::EPart::Controller] = 23;
			S.Parts[(int32)SS::EPart::Motor] = 20;
			S.Parts[(int32)SS::EPart::PhaseWire] = 7;
			S.Parts[(int32)SS::EPart::BatteryWire] = 8;
			S.Parts[(int32)SS::EPart::Connector] = 6;
			S.Cell = 5;
			S.Series = 26;
			S.Parallel = 11;
			S.Bms = 6;
			S.PhaseA = 380.f;
			S.BattA = 190.f;
			S.FieldWeakA = 120.f;
		}
		return S;
	}
}

ARaceManager::ARaceManager()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

int32 ARaceManager::NumRaces() const
{
	return RmNumRaces;
}

FVector ARaceManager::GetMarker(int32 Index) const
{
	return Markers.IsValidIndex(Index) ? Markers[Index] : FVector::ZeroVector;
}

bool ARaceManager::IsRaceWon(int32 Index) const
{
	AScooterPlayerController* PC = GetPC();
	return PC && (PC->GetProfileConst().RacesWon & (1u << (uint32)Index)) != 0;
}

AScooterPawn* ARaceManager::GetPlayer() const
{
	return Cast<AScooterPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
}

AScooterPlayerController* ARaceManager::GetPC() const
{
	return Cast<AScooterPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

void ARaceManager::BeginPlay()
{
	Super::BeginPlay();
	City = ScooterShapes::FindFirst<AScooterWorld>(GetWorld());
	if (!City.IsValid())
	{
		return;
	}
	const AScooterWorld* W = City.Get();
	const float M = 100.f;
	Markers.Reset();
	Markers.Add(W->GridPoint(1, 4) + FVector(-10.f, 10.f, 0.f) * M);
	Markers.Add(W->GridPoint(1, 6) + FVector(10.f, -10.f, 0.f) * M);
	Markers.Add(W->HighwayPoint(20.f) + FVector(0.f, -9.f, 0.f) * M);
	Markers.Add(W->GridPoint(2, 2) + FVector(10.f, 10.f, 0.f) * M);
	Markers.Add(W->HighwayPoint(35.f) + FVector(0.f, -9.f, 0.f) * M);
	Markers.Add(W->GridPoint(4, 4) + FVector(-10.f, -10.f, 0.f) * M);
	Markers.Add(W->HighwayPoint(50.f) + FVector(0.f, -9.f, 0.f) * M);
	Markers.Add(FVector(16.f, 72.f, 0.f) * M);
	Markers.Add(W->HighwayPoint(20.f) + FVector(0.f, 9.f, 0.f) * M);

	for (int32 i = 0; i < Markers.Num() && i < RmNumRaces; ++i)
	{
		const FRmRaceDef& R = RmRaces[i];
		const bool bWheelie = R.Kind == ERmKind::Wheelie;
		const FLinearColor Color = bWheelie ? FLinearColor(0.1f, 0.8f, 1.f) : FLinearColor(1.f, 0.45f, 0.05f);
		ScooterShapes::AddShape(this, SceneRoot, EScooterShape::Cylinder, Markers[i] + FVector(0.f, 0.f, 250.f), FVector(110.f, 110.f, 500.f), Color);
		ScooterShapes::AddShape(this, SceneRoot, EScooterShape::Sphere, Markers[i] + FVector(0.f, 0.f, 560.f), FVector(140.f, 140.f, 140.f), FLinearColor(1.f, 0.9f, 0.2f));
		ScooterShapes::AddLight(this, SceneRoot, Markers[i] + FVector(0.f, 0.f, 600.f), Color, 3000.f, 2000.f);
		const FString Label = bWheelie ? FString(R.Name) : FString::Printf(TEXT("%s\nvs %s"), R.Name, R.Opponent);
		for (int32 Side = 0; Side < 4; ++Side)
		{
			const float Yaw = (float)Side * 90.f;
			const FVector Off = FRotator(0.f, Yaw, 0.f).Vector() * 60.f;
			ScooterShapes::AddText(this, SceneRoot, Label, Markers[i] + Off + FVector(0.f, 0.f, 700.f), FRotator(0.f, Yaw, 0.f), 55.f, FColor(255, 230, 120));
		}
	}

	CheckpointMesh = ScooterShapes::AddShape(this, SceneRoot, EScooterShape::Cylinder, FVector::ZeroVector, FVector(160.f, 160.f, 1400.f), FLinearColor(1.f, 0.85f, 0.f));
	NextCheckpointMesh = ScooterShapes::AddShape(this, SceneRoot, EScooterShape::Cylinder, FVector::ZeroVector, FVector(90.f, 90.f, 800.f), FLinearColor(1.f, 0.4f, 0.f));
	CheckpointLight = ScooterShapes::AddLight(this, SceneRoot, FVector::ZeroVector, FLinearColor(1.f, 0.8f, 0.1f), 8000.f, 4000.f);
	CheckpointMesh->SetCastShadow(false);
	NextCheckpointMesh->SetCastShadow(false);
	UpdateCheckpoints();
}

void ARaceManager::BuildPath(int32 RaceIndex, TArray<FVector>& OutPath) const
{
	OutPath.Reset();
	if (!City.IsValid())
	{
		return;
	}
	const AScooterWorld* W = City.Get();
	auto AddNodes = [&OutPath, W](std::initializer_list<FIntPoint> Nodes)
	{
		for (const FIntPoint& N : Nodes)
		{
			OutPath.Add(W->GridPoint(N.X, N.Y));
		}
	};
	switch (RaceIndex)
	{
	case 0:
		OutPath.Add(W->GridPoint(1, 4));
		OutPath.Add(W->GridPoint(1, 4) + FVector(20000.f, 0.f, 0.f));
		break;
	case 1:
		AddNodes({ FIntPoint(1, 4), FIntPoint(1, 6), FIntPoint(3, 6), FIntPoint(3, 4), FIntPoint(1, 4), FIntPoint(1, 6), FIntPoint(3, 6), FIntPoint(3, 4), FIntPoint(1, 4) });
		break;
	case 2:
		OutPath.Add(W->HighwayPoint(20.f));
		OutPath.Add(W->HighwayPoint(420.f));
		break;
	case 3:
		AddNodes({ FIntPoint(2, 2), FIntPoint(2, 6), FIntPoint(6, 6), FIntPoint(6, 2), FIntPoint(2, 2) });
		break;
	case 4:
		OutPath.Add(W->HighwayPoint(20.f));
		OutPath.Add(W->HighwayPoint(1020.f));
		break;
	case 5:
		AddNodes({ FIntPoint(4, 4), FIntPoint(4, 8), FIntPoint(8, 8), FIntPoint(8, 0), FIntPoint(4, 0), FIntPoint(4, 4) });
		break;
	case 6:
		OutPath.Add(W->HighwayPoint(20.f));
		OutPath.Add(W->HighwayPoint(2020.f));
		break;
	default:
		break;
	}
}

bool ARaceManager::GetPrompt(const FVector& PlayerLoc, FString& OutText) const
{
	if (State != EScooterRaceState::Idle)
	{
		return false;
	}
	for (int32 i = 0; i < Markers.Num() && i < RmNumRaces; ++i)
	{
		if (FVector::Dist2D(PlayerLoc, Markers[i]) < 800.0)
		{
			const FRmRaceDef& R = RmRaces[i];
			if (R.Kind == ERmKind::Wheelie)
			{
				OutText = FString::Printf(TEXT("[E] %s  |  Halte %.0f m einen Wheelie in 60 s  |  Preis %s"), R.Name, R.Param,
					*AScooterPlayerController::FormatMoney(IsRaceWon(i) ? R.Prize * 0.4f : R.Prize));
			}
			else
			{
				OutText = FString::Printf(TEXT("[E] RENNEN: %s  |  Gegner: %s (%s)  |  Einsatz %s  ->  Gewinn %s"), R.Name, R.Opponent,
					SS::Vehicle(R.OpponentVehicle).Name, *AScooterPlayerController::FormatMoney(R.Stake), *AScooterPlayerController::FormatMoney(R.Prize));
			}
			return true;
		}
	}
	return false;
}

bool ARaceManager::TryStartNearby(AScooterPlayerController* PC)
{
	AScooterPawn* Pawn = GetPlayer();
	if (!PC || !Pawn || State != EScooterRaceState::Idle)
	{
		return false;
	}
	for (int32 i = 0; i < Markers.Num() && i < RmNumRaces; ++i)
	{
		if (FVector::Dist2D(Pawn->GetActorLocation(), Markers[i]) < 800.0)
		{
			if (Pawn->IsBroken())
			{
				PC->Notify(TEXT("Mit kaputtem Fahrzeug kannst du nicht starten. [R] Abschleppen."), FLinearColor(1.f, 0.4f, 0.2f), 3.f, false);
				return true;
			}
			if (!PC->SpendMoney(RmRaces[i].Stake))
			{
				PC->Notify(FString::Printf(TEXT("Zu wenig Geld f\u00fcr den Einsatz (%s)."), *AScooterPlayerController::FormatMoney(RmRaces[i].Stake)), FLinearColor(1.f, 0.3f, 0.3f), 3.f, false);
				return true;
			}
			StartRace(i, PC);
			return true;
		}
	}
	return false;
}

void ARaceManager::StartRace(int32 RaceIndex, AScooterPlayerController* PC)
{
	AScooterPawn* Pawn = GetPlayer();
	const FRmRaceDef& R = RmRaces[RaceIndex];
	ActiveRace = RaceIndex;
	RaceTime = 0.f;
	PlayerNext = 1;

	if (R.Kind == ERmKind::Wheelie)
	{
		Path.Reset();
		State = EScooterRaceState::Running;
		Timer = 60.f;
		PC->Notify(FString::Printf(TEXT("WHEELIE-CHALLENGE: %.0f m am St\u00fcck! Zur\u00fccklehnen (Leertaste) + Gas."), R.Param), FLinearColor(0.2f, 0.9f, 1.f), 4.f, true);
		UpdateCheckpoints();
		return;
	}

	BuildPath(RaceIndex, Path);
	if (Path.Num() < 2 || !Pawn)
	{
		State = EScooterRaceState::Idle;
		return;
	}
	const FVector Start = Path[0];
	const FVector Dir = (Path[1] - Path[0]).GetSafeNormal2D();
	const FVector Right = FVector(-Dir.Y, Dir.X, 0.f);
	const float StartYaw = (float)Dir.Rotation().Yaw;
	Pawn->TeleportTo(Start + Right * 300.f, StartYaw);
	Pawn->SetRaceFrozen(true);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Bot = GetWorld()->SpawnActor<ARacerBot>(ARacerBot::StaticClass(), FTransform(Start - Right * 300.f), Params);
	if (Bot)
	{
		TArray<FVector> BotPath = Path;
		BotPath.RemoveAt(0);
		for (FVector& P : BotPath)
		{
			P -= Right * 150.f;
		}
		Bot->Setup(R.OpponentVehicle, RmOpponentSetup(RaceIndex), BotPath, R.Skill, ScooterShapes::Hex(R.JacketColor),
			FLinearColor(0.9f, 0.9f, 0.9f), Start - Right * 300.f, StartYaw);
	}
	State = EScooterRaceState::Countdown;
	Timer = 3.f;
	PC->Notify(FString::Printf(TEXT("%s gegen %s"), R.Name, R.Opponent), FLinearColor(1.f, 0.6f, 0.1f), 3.f, true);
	UpdateCheckpoints();
}

void ARaceManager::FinishRace(bool bWon, const FString& Text)
{
	AScooterPlayerController* PC = GetPC();
	AScooterPawn* Pawn = GetPlayer();
	if (Pawn)
	{
		Pawn->SetRaceFrozen(false);
	}
	if (ActiveRace >= 0 && PC)
	{
		const FRmRaceDef& R = RmRaces[ActiveRace];
		if (bWon)
		{
			const bool bFirst = !IsRaceWon(ActiveRace);
			const float Prize = (R.Kind == ERmKind::Wheelie && !bFirst) ? R.Prize * 0.4f : R.Prize;
			PC->GetProfile().RacesWon |= (1u << (uint32)ActiveRace);
			PC->AddMoney(Prize, Text);
			PC->SaveProfile();
		}
		else
		{
			PC->Notify(Text, FLinearColor(1.f, 0.3f, 0.3f), 4.f, true);
		}
	}
	ResultText = Text;
	bResultWon = bWon;
	State = EScooterRaceState::Result;
	Timer = 4.f;
	Path.Reset();
	UpdateCheckpoints();
}

void ARaceManager::AbortRace(const FString& Reason)
{
	if (State == EScooterRaceState::Countdown || State == EScooterRaceState::Running)
	{
		FinishRace(false, Reason);
	}
}

void ARaceManager::UpdateCheckpoints()
{
	const bool bShow = State == EScooterRaceState::Running || State == EScooterRaceState::Countdown;
	const bool bHasCp = bShow && Path.IsValidIndex(PlayerNext);
	if (CheckpointMesh)
	{
		CheckpointMesh->SetVisibility(bHasCp);
		if (bHasCp)
		{
			CheckpointMesh->SetWorldLocation(Path[PlayerNext] + FVector(0.f, 0.f, 700.f));
		}
	}
	if (CheckpointLight)
	{
		CheckpointLight->SetVisibility(bHasCp);
		if (bHasCp)
		{
			CheckpointLight->SetWorldLocation(Path[PlayerNext] + FVector(0.f, 0.f, 300.f));
		}
	}
	const bool bHasNext = bShow && Path.IsValidIndex(PlayerNext + 1);
	if (NextCheckpointMesh)
	{
		NextCheckpointMesh->SetVisibility(bHasNext);
		if (bHasNext)
		{
			NextCheckpointMesh->SetWorldLocation(Path[PlayerNext + 1] + FVector(0.f, 0.f, 400.f));
		}
	}
}

void ARaceManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	AScooterPawn* Pawn = GetPlayer();
	AScooterPlayerController* PC = GetPC();
	if (!Pawn || !PC)
	{
		return;
	}
	TickRadar(Pawn, PC, Dt);

	switch (State)
	{
	case EScooterRaceState::Countdown:
	{
		const int32 Before = FMath::CeilToInt(Timer);
		Timer -= Dt;
		const int32 After = FMath::CeilToInt(Timer);
		if (After != Before && After > 0)
		{
			PC->Notify(FString::Printf(TEXT("%d"), After), FLinearColor(1.f, 1.f, 0.2f), 0.9f, true);
		}
		if (Timer <= 0.f)
		{
			State = EScooterRaceState::Running;
			Pawn->SetRaceFrozen(false);
			if (Bot)
			{
				Bot->Go();
			}
			PC->Notify(TEXT("LOS!"), FLinearColor(0.2f, 1.f, 0.3f), 1.2f, true);
		}
		break;
	}
	case EScooterRaceState::Running:
	{
		RaceTime += Dt;
		const FRmRaceDef& R = RmRaces[ActiveRace];
		if (R.Kind == ERmKind::Wheelie)
		{
			Timer -= Dt;
			if (Pawn->GetWheelieDistance() >= R.Param)
			{
				FinishRace(true, FString::Printf(TEXT("WHEELIE-CHALLENGE GESCHAFFT! %.0f m"), R.Param));
			}
			else if (Timer <= 0.f)
			{
				FinishRace(false, TEXT("Zeit abgelaufen - Wheelie-Challenge verloren."));
			}
			break;
		}
		if (Pawn->IsBroken())
		{
			FinishRace(false, TEXT("Fahrzeug kaputt - Rennen verloren!"));
			break;
		}
		if (Path.IsValidIndex(PlayerNext))
		{
			const float D = (float)FVector::Dist2D(Pawn->GetActorLocation(), Path[PlayerNext]);
			if (D < 1200.f)
			{
				++PlayerNext;
				UpdateCheckpoints();
			}
			else if (D > 80000.f)
			{
				FinishRace(false, TEXT("Vom Kurs abgekommen - Rennen verloren."));
				break;
			}
		}
		if (PlayerNext >= Path.Num())
		{
			FinishRace(true, FString::Printf(TEXT("SIEG! %s geschlagen in %.1f s"), R.Opponent, RaceTime));
			break;
		}
		if (Bot && Bot->HasFinished())
		{
			FinishRace(false, FString::Printf(TEXT("%s war schneller. Einsatz weg!"), R.Opponent));
			break;
		}
		if (RaceTime > 300.f)
		{
			FinishRace(false, TEXT("Zeit abgelaufen."));
		}
		break;
	}
	case EScooterRaceState::Result:
	{
		Timer -= Dt;
		if (Timer <= 0.f)
		{
			State = EScooterRaceState::Idle;
			ActiveRace = -1;
			if (Bot)
			{
				Bot->Destroy();
				Bot = nullptr;
			}
			UpdateCheckpoints();
		}
		break;
	}
	default:
		break;
	}
}

void ARaceManager::TickRadar(AScooterPawn* Pawn, AScooterPlayerController* PC, float Dt)
{
	if (!City.IsValid())
	{
		return;
	}
	RadarCooldown -= Dt;
	const FVector Gate = City->GetRadarGate();
	const FVector P = Pawn->GetActorLocation();
	const float X = (float)P.X;
	// Teleports (Abschleppen, Rennstart) zaehlen nicht als Durchfahrt
	const bool bCrossed = (PrevPlayerX < (float)Gate.X) != (X < (float)Gate.X) && FMath::Abs(X - PrevPlayerX) < 5000.f;
	PrevPlayerX = X;
	if (!bCrossed || RadarCooldown > 0.f || FMath::Abs((float)P.Y) > 1500.f)
	{
		return;
	}
	RadarCooldown = 2.f;
	const float Kmh = Pawn->GetSpeedKmh();
	SS::FProfile& Prof = PC->GetProfile();
	FString Text = FString::Printf(TEXT("RADAR: %.1f km/h"), Kmh);
	float Reward = 0.f;
	for (int32 i = 0; i < 5; ++i)
	{
		const uint32 Bit = 1u << (uint32)i;
		if (Kmh >= RmRadarKmh[i] && (Prof.RadarMask & Bit) == 0)
		{
			Prof.RadarMask |= Bit;
			Reward += RmRadarPrize[i];
			Text += FString::Printf(TEXT("  |  %.0f-km/h-Marke geknackt!"), RmRadarKmh[i]);
		}
	}
	if (Reward > 0.f)
	{
		PC->AddMoney(Reward, Text);
		PC->SaveProfile();
	}
	else
	{
		PC->Notify(Text, FLinearColor(1.f, 0.3f, 0.3f), 2.5f, true);
	}
}

FRaceHudInfo ARaceManager::GetHudInfo() const
{
	FRaceHudInfo Info;
	if (State == EScooterRaceState::Idle || ActiveRace < 0)
	{
		return Info;
	}
	const FRmRaceDef& R = RmRaces[ActiveRace];
	Info.bActive = true;
	Info.Title = R.Name;
	if (State == EScooterRaceState::Countdown)
	{
		Info.Countdown = Timer;
		Info.Line1 = FString::Printf(TEXT("Gegner: %s auf %s"), R.Opponent, SS::Vehicle(R.OpponentVehicle).Name);
	}
	else if (State == EScooterRaceState::Running)
	{
		if (R.Kind == ERmKind::Wheelie)
		{
			AScooterPawn* Pawn = GetPlayer();
			const float Dist = Pawn ? Pawn->GetWheelieDistance() : 0.f;
			Info.Line1 = FString::Printf(TEXT("Wheelie: %.0f / %.0f m"), Dist, R.Param);
			Info.Line2 = FString::Printf(TEXT("Zeit: %.0f s"), FMath::Max(0.f, Timer));
		}
		else
		{
			const int32 Total = FMath::Max(1, Path.Num() - 1);
			const int32 Done = FMath::Clamp(PlayerNext - 1, 0, Total);
			const int32 BotDone = Bot ? Bot->GetNextIndex() : 0;
			const bool bLeading = Done > BotDone || (Done == BotDone && Bot && Path.IsValidIndex(PlayerNext) && GetPlayer() &&
				FVector::Dist2D(GetPlayer()->GetActorLocation(), Path[PlayerNext]) < FVector::Dist2D(Bot->GetActorLocation(), Path[PlayerNext]));
			Info.Line1 = FString::Printf(TEXT("Checkpoint %d / %d   |   %s   |   %.1f s"), Done, Total, bLeading ? TEXT("PLATZ 1") : TEXT("PLATZ 2"), RaceTime);
			Info.Line2 = FString::Printf(TEXT("%s: %.0f km/h"), R.Opponent, Bot ? Bot->GetSpeedKmh() : 0.f);
			if (Path.IsValidIndex(PlayerNext))
			{
				Info.bHasTarget = true;
				Info.Target = Path[PlayerNext];
			}
		}
	}
	else
	{
		Info.Line1 = ResultText;
		Info.Line2 = bResultWon ? TEXT("Gewonnen!") : TEXT("Verloren.");
	}
	return Info;
}
