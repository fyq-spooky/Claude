#include "ScooterPlayerController.h"
#include "ScooterPawn.h"
#include "ScooterWorld.h"
#include "ScooterShapes.h"
#include "ScooterSaveGame.h"
#include "PoliceManager.h"
#include "RaceManager.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"

namespace SS = ScooterSim;

namespace
{
	constexpr int32 PcTabVehicles = 0;
	constexpr int32 PcTabParts = 1;
	constexpr int32 PcTabBattery = 2;
	constexpr int32 PcTabSettings = 3;
	constexpr int32 PcTabWorkshop = 4;
	const TCHAR* PcSaveSlot = TEXT("ScooterSim");

	const FLinearColor PcGreen(0.35f, 1.f, 0.45f);
	const FLinearColor PcRed(1.f, 0.3f, 0.3f);
	const FLinearColor PcOrange(1.f, 0.62f, 0.15f);
	const FLinearColor PcYellow(1.f, 0.9f, 0.3f);
	const FLinearColor PcCyan(0.35f, 0.85f, 1.f);
	const FLinearColor PcGrey(0.6f, 0.6f, 0.62f);

	FLinearColor PcTempColor(SS::EThermal Part, float T)
	{
		const SS::FThermalSpec& Spec = SS::ThermalSpec(Part);
		if (T >= Spec.FailC) return PcRed;
		if (T >= Spec.WarnC) return PcOrange;
		return PcGreen;
	}

	FLinearColor PcLoadColor(float Ratio)
	{
		if (Ratio <= 1.05f) return PcGreen;
		if (Ratio <= 1.6f) return PcOrange;
		return PcRed;
	}

	const TCHAR* PcControllerKind(SS::EControllerKind Kind)
	{
		switch (Kind)
		{
		case SS::EControllerKind::Cheap: return TEXT("Billig-Sinus");
		case SS::EControllerKind::Vesc: return TEXT("VESC");
		case SS::EControllerKind::Fardriver: return TEXT("Fardriver");
		default: return TEXT("Serie");
		}
	}

	const TCHAR* PcTuningApp(SS::EControllerKind Kind)
	{
		switch (Kind)
		{
		case SS::EControllerKind::Vesc: return TEXT("VESC Tool");
		case SS::EControllerKind::Fardriver: return TEXT("Fardriver-App");
		case SS::EControllerKind::Cheap: return TEXT("Programmierkabel");
		default: return TEXT("Tacho P-Settings");
		}
	}

}

AScooterPlayerController::AScooterPlayerController()
{
	bShowMouseCursor = false;
	SS::InitProfile(Profile);
	PartPreview.Init(0, SS::NumPartCategories);
	WarnCooldown.Init(0.f, SS::NumThermal);
}

void AScooterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureProfile();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AScooterPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bProfileReady)
	{
		SaveProfile();
	}
	Super::EndPlay(EndPlayReason);
}

void AScooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	EnsureProfile();
	if (AScooterPawn* Scooter = Cast<AScooterPawn>(InPawn))
	{
		Scooter->SetProfile(&Profile);
	}
}

SS::FProfile& AScooterPlayerController::GetProfile()
{
	EnsureProfile();
	return Profile;
}

void AScooterPlayerController::EnsureProfile()
{
	if (!bProfileReady)
	{
		bProfileReady = true;
		LoadProfile();
	}
}

void AScooterPlayerController::LoadProfile()
{
	SS::InitProfile(Profile);
	bool bLoaded = false;
	if (UGameplayStatics::DoesSaveGameExist(PcSaveSlot, 0))
	{
		UScooterSaveGame* Save = Cast<UScooterSaveGame>(UGameplayStatics::LoadGameFromSlot(PcSaveSlot, 0));
		if (Save && Save->ToProfile(Profile))
		{
			bLoaded = true;
			Notify(TEXT("Spielstand geladen. Viel Spa\u00df!"), PcGreen, 3.f, false);
		}
		else
		{
			SS::InitProfile(Profile);
			Notify(TEXT("Alter Spielstand passt nicht mehr - neues Spiel gestartet."), PcOrange, 4.f, false);
		}
	}
	if (!bLoaded)
	{
		Notify(TEXT("Neues Spiel: 300 \u20ac und ein Billigroller. Ab in die Garage [E]!"), PcYellow, 6.f, true);
	}
	LastAnnouncedRecord = Profile.BestTopSpeedKmh;
}

void AScooterPlayerController::SaveProfile()
{
	UScooterSaveGame* Save = Cast<UScooterSaveGame>(UGameplayStatics::CreateSaveGameObject(UScooterSaveGame::StaticClass()));
	if (Save)
	{
		Save->FromProfile(Profile);
		UGameplayStatics::SaveGameToSlot(Save, PcSaveSlot, 0);
	}
}

AScooterPawn* AScooterPlayerController::GetScooter() const
{
	return Cast<AScooterPawn>(GetPawn());
}

AScooterWorld* AScooterPlayerController::GetCity() const
{
	return ScooterShapes::FindFirst<AScooterWorld>(GetWorld());
}

APoliceManager* AScooterPlayerController::GetPolice() const
{
	return ScooterShapes::FindFirst<APoliceManager>(GetWorld());
}

ARaceManager* AScooterPlayerController::GetRaces() const
{
	return ScooterShapes::FindFirst<ARaceManager>(GetWorld());
}

// ---------------------------------------------------------------------------------------------
// Geld & Meldungen

FString AScooterPlayerController::FormatMoney(float Amount)
{
	const int64 Value = (int64)FMath::RoundToDouble(FMath::Abs((double)Amount));
	const FString Digits = FString::Printf(TEXT("%lld"), Value);
	FString Out;
	int32 Count = 0;
	for (int32 i = Digits.Len() - 1; i >= 0; --i)
	{
		Out.InsertAt(0, Digits[i]);
		++Count;
		if (Count % 3 == 0 && i > 0)
		{
			Out.InsertAt(0, TEXT('.'));
		}
	}
	return FString(Amount < -0.5f ? TEXT("-") : TEXT("")) + Out + TEXT(" \u20ac");
}

void AScooterPlayerController::Notify(const FString& Text, const FLinearColor& Color, float Duration, bool bBig)
{
	if (bBig)
	{
		Notices.RemoveAll([](const FScooterNotice& N) { return N.bBig; });
	}
	FScooterNotice N;
	N.Text = Text;
	N.Color = Color;
	N.Duration = FMath::Max(Duration, 0.1f);
	N.TimeLeft = N.Duration;
	N.bBig = bBig;
	Notices.Add(N);
	while (Notices.Num() > 7)
	{
		Notices.RemoveAt(0);
	}
}

void AScooterPlayerController::UpdateNotices(float Dt)
{
	for (FScooterNotice& N : Notices)
	{
		N.TimeLeft -= Dt;
	}
	Notices.RemoveAll([](const FScooterNotice& N) { return N.TimeLeft <= 0.f; });
}

void AScooterPlayerController::AddMoney(float Amount, const FString& Reason)
{
	Profile.Money += Amount;
	Notify(FString::Printf(TEXT("%s   %s%s"), *Reason, Amount >= 0.f ? TEXT("+") : TEXT(""), *FormatMoney(Amount)),
		Amount >= 0.f ? PcGreen : PcRed, 3.5f, true);
}

bool AScooterPlayerController::SpendMoney(float Amount)
{
	if (Amount <= 0.f)
	{
		return true;
	}
	if (Profile.Money < Amount)
	{
		return false;
	}
	Profile.Money -= Amount;
	return true;
}

void AScooterPlayerController::OnVehicleFailure(SS::EFailure Failure)
{
	SS::OnFailure(Profile, Profile.Current, Failure);
	Notify(FString::Printf(TEXT("%s!"), SS::FailureName(Failure)), PcRed, 5.f, true);
	Notify(TEXT("[R] Abschleppen lassen - in der Werkstatt reparieren oder Serienteil einbauen."), PcOrange, 6.f, false);
	SaveProfile();
}

void AScooterPlayerController::OnWheelieEnded(float DistM, float TimeS, float AvgSpeedMs)
{
	Profile.TotalWheelieM += DistM;
	const bool bRecord = DistM > Profile.BestWheelieM && DistM > 10.f;
	Profile.BestWheelieM = FMath::Max(Profile.BestWheelieM, DistM);
	const float Money = FMath::RoundToFloat(DistM * 0.5f * (1.f + AvgSpeedMs / 10.f));
	if (Money < 1.f)
	{
		return;
	}
	const FString Text = FString::Printf(TEXT("WHEELIE %.0f m (%.1f s)%s"), DistM, TimeS, bRecord ? TEXT(" - NEUER REKORD!") : TEXT(""));
	if (DistM > 15.f || bRecord)
	{
		AddMoney(Money, Text);
	}
	else
	{
		Profile.Money += Money;
		Notify(Text + TEXT("   +") + FormatMoney(Money), PcCyan, 2.f, false);
	}
}

void AScooterPlayerController::OnStuntLanded(float AirTimeS, int32 Flips, float SpeedKmh)
{
	if (AirTimeS < 0.6f && Flips == 0)
	{
		return;
	}
	float Money = FMath::RoundToFloat(AirTimeS * 20.f * (1.f + SpeedKmh / 60.f));
	FString Text = FString::Printf(TEXT("AIRTIME %.1f s"), AirTimeS);
	if (Flips > 0)
	{
		Money += 400.f * (float)Flips;
		Text = FString::Printf(TEXT("BACKFLIP x%d!  "), Flips) + Text;
	}
	else if (Flips < 0)
	{
		Money += 500.f * (float)-Flips;
		Text = FString::Printf(TEXT("FRONTFLIP x%d!  "), -Flips) + Text;
	}
	AddMoney(Money, Text);
}

void AScooterPlayerController::OnCrash(const FString& Reason)
{
	Notify(Reason, PcRed, 2.5f, true);
}

void AScooterPlayerController::OnBusted(float Fine)
{
	const float Paid = FMath::Min(Profile.Money, Fine);
	Profile.Money -= Paid;
	Profile.Busts++;
	Notify(FString::Printf(TEXT("GESCHNAPPT! Strafe: %s"), *FormatMoney(Paid)), PcRed, 5.f, true);
	if (ARaceManager* Races = GetRaces())
	{
		Races->AbortRace(TEXT("Von der Polizei gestoppt - Rennen verloren."));
	}
	AScooterPawn* Scooter = GetScooter();
	AScooterWorld* City = GetCity();
	if (Scooter && City)
	{
		Scooter->TeleportTo(City->GetSpawnLocation(), City->GetSpawnYaw());
	}
	SaveProfile();
}

// ---------------------------------------------------------------------------------------------
// Eingabe

bool AScooterPlayerController::KeyDown(const FKey& Key) const
{
	return IsInputKeyDown(Key);
}

bool AScooterPlayerController::KeyEdge(const FKey& Key)
{
	return VirtualEdge(Key.GetFName(), IsInputKeyDown(Key));
}

bool AScooterPlayerController::VirtualEdge(FName Name, bool bDown)
{
	bool& Prev = PrevKeys.FindOrAdd(Name);
	const bool bEdge = bDown && !Prev;
	Prev = bDown;
	return bEdge;
}

void AScooterPlayerController::ReadButtons(float Dt)
{
	const float LX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float LY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

	const bool bHeldUp = KeyDown(EKeys::W) || KeyDown(EKeys::Up) || KeyDown(EKeys::Gamepad_DPad_Up) || LY > 0.6f;
	const bool bHeldDown = KeyDown(EKeys::S) || KeyDown(EKeys::Down) || KeyDown(EKeys::Gamepad_DPad_Down) || LY < -0.6f;
	const bool bHeldLeft = KeyDown(EKeys::A) || KeyDown(EKeys::Left) || KeyDown(EKeys::Gamepad_DPad_Left) || LX < -0.6f;
	const bool bHeldRight = KeyDown(EKeys::D) || KeyDown(EKeys::Right) || KeyDown(EKeys::Gamepad_DPad_Right) || LX > 0.6f;
	const bool bEdgeUp = VirtualEdge(TEXT("NavUp"), bHeldUp);
	const bool bEdgeDown = VirtualEdge(TEXT("NavDown"), bHeldDown);
	const bool bEdgeLeft = VirtualEdge(TEXT("NavLeft"), bHeldLeft);
	const bool bEdgeRight = VirtualEdge(TEXT("NavRight"), bHeldRight);

	auto Repeat = [Dt](bool bHeld, bool bEdge, float& Timer) -> bool
	{
		if (!bHeld)
		{
			Timer = 0.f;
			return false;
		}
		if (bEdge)
		{
			Timer = -0.35f;
			return true;
		}
		Timer += Dt;
		if (Timer >= 0.06f)
		{
			Timer = 0.f;
			return true;
		}
		return false;
	};
	Btn.Up = Repeat(bHeldUp, bEdgeUp, RepeatUp);
	Btn.Down = Repeat(bHeldDown, bEdgeDown, RepeatDown);
	Btn.Left = Repeat(bHeldLeft, bEdgeLeft, RepeatLeft);
	Btn.Right = Repeat(bHeldRight, bEdgeRight, RepeatRight);

	const bool bShift = KeyDown(EKeys::LeftShift) || KeyDown(EKeys::RightShift);
	const bool bEnter = KeyEdge(EKeys::Enter);
	const bool bE = KeyEdge(EKeys::E);
	const bool bPadA = KeyEdge(EKeys::Gamepad_FaceButton_Bottom);
	const bool bSpace = KeyEdge(EKeys::SpaceBar);
	const bool bBackspace = KeyEdge(EKeys::BackSpace);
	const bool bEscape = KeyEdge(EKeys::Escape);
	const bool bPadB = KeyEdge(EKeys::Gamepad_FaceButton_Right);
	const bool bTab = KeyEdge(EKeys::Tab);
	const bool bQ = KeyEdge(EKeys::Q);
	const bool bRB = KeyEdge(EKeys::Gamepad_RightShoulder);
	const bool bLB = KeyEdge(EKeys::Gamepad_LeftShoulder);
	const bool bT = KeyEdge(EKeys::T);
	const bool bPadY = KeyEdge(EKeys::Gamepad_FaceButton_Top);
	const bool bR = KeyEdge(EKeys::R);
	const bool bPadX = KeyEdge(EKeys::Gamepad_FaceButton_Left);
	const bool bH = KeyEdge(EKeys::H);
	const bool bStart = KeyEdge(EKeys::Gamepad_Special_Right);
	const bool bM = KeyEdge(EKeys::M);
	const bool bBackBtn = KeyEdge(EKeys::Gamepad_Special_Left);

	Btn.Confirm = bEnter || bE || bPadA || bSpace;
	Btn.Interact = bE || bPadA;
	Btn.Back = bBackspace || bEscape || bPadB;
	Btn.TabNext = (bTab && !bShift) || bRB;
	Btn.TabPrev = (bTab && bShift) || bQ || bLB;
	Btn.Tuning = bT || bPadY;
	Btn.Reset = bR || bPadX;
	Btn.Help = bH || bStart;
	Btn.Map = bM || bBackBtn;
	Btn.Fine = bShift;
}

void AScooterPlayerController::HandleDriving(float Dt, AScooterPawn* Scooter)
{
	if (!Scooter)
	{
		return;
	}
	const bool bGas = KeyDown(EKeys::W) || KeyDown(EKeys::Up);
	const bool bBrake = KeyDown(EKeys::S) || KeyDown(EKeys::Down);
	KeyThrottle = bGas ? FMath::Min(1.f, KeyThrottle + 3.5f * Dt) : FMath::Max(0.f, KeyThrottle - 6.f * Dt);
	KeyBrake = bBrake ? FMath::Min(1.f, KeyBrake + 6.f * Dt) : 0.f;
	const float SteerTarget = ((KeyDown(EKeys::D) || KeyDown(EKeys::Right)) ? 1.f : 0.f) - ((KeyDown(EKeys::A) || KeyDown(EKeys::Left)) ? 1.f : 0.f);
	KeySteer = FMath::FInterpConstantTo(KeySteer, SteerTarget, Dt, SteerTarget == 0.f ? 7.f : 4.f);
	const float LeanTarget = (KeyDown(EKeys::SpaceBar) ? 1.f : 0.f) - ((KeyDown(EKeys::LeftControl) || KeyDown(EKeys::C)) ? 1.f : 0.f);
	KeyLean = FMath::FInterpConstantTo(KeyLean, LeanTarget, Dt, 5.f);

	auto Dead = [](float V) { return FMath::Abs(V) < 0.12f ? 0.f : V; };
	FScooterDriveInput In;
	In.Throttle = FMath::Max(KeyThrottle, GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis));
	In.Brake = FMath::Max(KeyBrake, GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis));
	In.Steer = FMath::Clamp(KeySteer + Dead(GetInputAnalogKeyState(EKeys::Gamepad_LeftX)), -1.f, 1.f);
	In.Lean = FMath::Clamp(KeyLean - Dead(GetInputAnalogKeyState(EKeys::Gamepad_RightY)), -1.f, 1.f);
	Scooter->SetDriveInput(In);
}

void AScooterPlayerController::HandleWorldButtons(AScooterPawn* Scooter)
{
	if (Btn.Help)
	{
		bShowHelp = !bShowHelp;
		HelpTimer = 1000000.f;
	}
	if (Btn.Map)
	{
		bBigMap = !bBigMap;
	}
	if (!Scooter)
	{
		return;
	}
	ARaceManager* Races = GetRaces();
	APoliceManager* Police = GetPolice();
	AScooterWorld* City = GetCity();
	const bool bRace = Races && Races->IsRaceActive();
	const bool bChase = Police && Police->IsChasing();

	if (Btn.Interact)
	{
		FString RacePrompt;
		if (Races && !bRace && Races->GetPrompt(Scooter->GetActorLocation(), RacePrompt))
		{
			if (bChase)
			{
				Notify(TEXT("Erst die Polizei abh\u00e4ngen!"), PcRed, 2.5f, false);
			}
			else
			{
				Races->TryStartNearby(this);
			}
		}
		else if (City && !bRace && City->IsInGarage(Scooter->GetActorLocation()))
		{
			if (bChase)
			{
				Notify(TEXT("Die Polizei ist hinter dir her! Erst abh\u00e4ngen."), PcRed, 2.5f, false);
			}
			else
			{
				OpenMenu(EScooterMenu::Garage, PcTabVehicles);
			}
		}
	}
	if (Btn.Tuning)
	{
		if (bRace)
		{
			Notify(TEXT("Nicht w\u00e4hrend des Rennens!"), PcOrange, 2.f, false);
		}
		else if (Scooter->GetSpeedKmh() > 3.f || Scooter->IsAirborne())
		{
			Notify(TEXT("Zum Einstellen erst anhalten."), PcOrange, 2.f, false);
		}
		else
		{
			OpenMenu(EScooterMenu::TuningApp, PcTabSettings);
		}
	}
	if (Btn.Reset)
	{
		if (Scooter->IsBroken())
		{
			if (bChase)
			{
				Notify(TEXT("W\u00e4hrend einer Verfolgung kommt kein Abschleppwagen!"), PcRed, 2.5f, false);
			}
			else
			{
				if (bRace)
				{
					Races->AbortRace(TEXT("Rennen abgebrochen."));
				}
				TowToGarage();
			}
		}
		else if (!Scooter->IsCrashed())
		{
			Scooter->ResetToLastSafe();
		}
	}
}

void AScooterPlayerController::TowToGarage()
{
	AScooterPawn* Scooter = GetScooter();
	AScooterWorld* City = GetCity();
	if (!Scooter || !City)
	{
		return;
	}
	const float Cost = 30.f;
	if (Profile.Money >= Cost)
	{
		Profile.Money -= Cost;
		Notify(TEXT("Abgeschleppt in die Garage (-30 \u20ac)"), PcOrange, 3.f, false);
	}
	else
	{
		Notify(TEXT("Abgeschleppt (gratis - du bist pleite)"), PcOrange, 3.f, false);
	}
	Scooter->TeleportTo(City->GetSpawnLocation(), City->GetSpawnYaw());
	OpenMenu(EScooterMenu::Garage, PcTabWorkshop);
}

void AScooterPlayerController::UpdateWarnings(float Dt, AScooterPawn* Scooter)
{
	DerateCooldown -= Dt;
	RecordCooldown -= Dt;
	for (float& W : WarnCooldown)
	{
		W -= Dt;
	}
	if (bShowHelp)
	{
		HelpTimer -= Dt;
		if (HelpTimer <= 0.f)
		{
			bShowHelp = false;
		}
	}
	AutoSaveTimer -= Dt;
	if (AutoSaveTimer <= 0.f)
	{
		AutoSaveTimer = 60.f;
		SaveProfile();
	}
	if (!Scooter || Menu != EScooterMenu::None)
	{
		return;
	}
	const SS::FVehicleState& St = Profile.States[Profile.Current];
	for (int32 i = 0; i < SS::NumThermal; ++i)
	{
		const SS::FThermalSpec& Spec = SS::ThermalSpec((SS::EThermal)i);
		if (St.Temp[i] >= Spec.WarnC && WarnCooldown[i] <= 0.f && St.Failure == SS::EFailure::None)
		{
			WarnCooldown[i] = 5.f;
			Notify(FString::Printf(TEXT("HEISS: %s %.0f \u00b0C (kaputt ab %.0f \u00b0C)"), Spec.Name, St.Temp[i], Spec.FailC), PcOrange, 2.5f, false);
		}
	}
	if (Scooter->GetTelemetry().bThermalDerate && DerateCooldown <= 0.f)
	{
		DerateCooldown = 6.f;
		Notify(TEXT("Temperaturschutz drosselt die Leistung"), PcYellow, 2.5f, false);
	}
	const float Kmh = Scooter->GetSpeedKmh();
	if (!Scooter->IsAirborne() && !Scooter->IsFrozen() && Kmh > Profile.BestTopSpeedKmh)
	{
		Profile.BestTopSpeedKmh = Kmh;
		if (Kmh > 20.f && Kmh - LastAnnouncedRecord >= 5.f && RecordCooldown <= 0.f)
		{
			LastAnnouncedRecord = Kmh;
			RecordCooldown = 3.f;
			Notify(FString::Printf(TEXT("Neuer Topspeed-Rekord: %.0f km/h"), Kmh), PcCyan, 2.f, false);
		}
	}
	if (!bSpeedTipShown && Kmh > APoliceManager::LegalKmh)
	{
		bSpeedTipShown = true;
		Notify(TEXT("\u00dcber 30 km/h jagt dich die Polizei, wenn sie dich sieht!"), PcYellow, 4.f, false);
	}
}

void AScooterPlayerController::UpdatePrompt(AScooterPawn* Scooter)
{
	Prompt.Reset();
	if (!Scooter || Menu != EScooterMenu::None)
	{
		return;
	}
	if (ARaceManager* Races = GetRaces())
	{
		FString RacePrompt;
		if (Races->GetPrompt(Scooter->GetActorLocation(), RacePrompt))
		{
			Prompt = RacePrompt;
			return;
		}
	}
	if (Scooter->IsBroken())
	{
		Prompt = FString::Printf(TEXT("%s  -  [R] Abschleppen in die Garage (30 \u20ac)"), SS::FailureName(Profile.States[Profile.Current].Failure));
		return;
	}
	AScooterWorld* City = GetCity();
	if (City && City->IsInGarage(Scooter->GetActorLocation()))
	{
		Prompt = TEXT("[E] Garage: Shop, Tuning & Werkstatt");
	}
}

void AScooterPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	EnsureProfile();
	const float Dt = FMath::Min(DeltaTime, 0.05f);
	UpdateNotices(Dt);
	ReadButtons(Dt);
	AScooterPawn* Scooter = GetScooter();
	if (Menu != EScooterMenu::None)
	{
		if (Scooter)
		{
			Scooter->SetDriveInput(FScooterDriveInput());
		}
		HandleMenuInput();
	}
	else
	{
		HandleDriving(Dt, Scooter);
		HandleWorldButtons(Scooter);
	}
	UpdateWarnings(Dt, Scooter);
	UpdatePrompt(Scooter);
}

// ---------------------------------------------------------------------------------------------
// Menue

void AScooterPlayerController::OpenMenu(EScooterMenu InMenu, int32 Tab)
{
	Menu = InMenu;
	MenuTab = Tab;
	MenuRow = 0;
	KeyThrottle = 0.f;
	KeyBrake = 0.f;
	if (AScooterPawn* Scooter = GetScooter())
	{
		Scooter->SetMenuFrozen(true);
		Scooter->SetDriveInput(FScooterDriveInput());
	}
	if (InMenu == EScooterMenu::Garage)
	{
		SS::FVehicleState& St = Profile.States[Profile.Current];
		const SS::FVehicleSetup& S = Profile.Setups[Profile.Current];
		const SS::FPackInfo Pack = SS::GetPackInfo(S.Cell, S.Series, S.Parallel, S.Bms);
		if (SS::StateOfCharge(Pack, St.AhUsed) < 0.99f)
		{
			St.AhUsed = 0.f;
			Notify(TEXT("Akku in der Garage vollgeladen."), PcGreen, 2.f, false);
		}
	}
	ResetPreviews();
	RefreshMenu();
}

void AScooterPlayerController::CloseMenu()
{
	Menu = EScooterMenu::None;
	if (AScooterPawn* Scooter = GetScooter())
	{
		Scooter->SetMenuFrozen(false);
	}
	SaveProfile();
}

void AScooterPlayerController::ResetPreviews()
{
	const SS::FVehicleSetup& S = Profile.Setups[Profile.Current];
	for (int32 c = 0; c < SS::NumPartCategories; ++c)
	{
		PartPreview[c] = S.Parts[c];
	}
	BatCell = S.Cell;
	BatSeries = S.Series;
	BatParallel = S.Parallel;
	BatBms = S.Bms;
}

TArray<int32> AScooterPlayerController::CurrentTabs() const
{
	if (Menu == EScooterMenu::TuningApp)
	{
		return { PcTabSettings };
	}
	return { PcTabVehicles, PcTabParts, PcTabBattery, PcTabSettings, PcTabWorkshop };
}

TArray<SS::EPart> AScooterPlayerController::PartCategoriesForVehicle() const
{
	const SS::FVehicleDef& Def = SS::Vehicle(Profile.Current);
	TArray<SS::EPart> Out;
	for (int32 c = 0; c < SS::NumPartCategories; ++c)
	{
		const SS::EPart Cat = (SS::EPart)c;
		if (Cat == SS::EPart::Sprocket && Def.Class != SS::EVehicleClass::Moto)
		{
			continue;
		}
		Out.Add(Cat);
	}
	return Out;
}

void AScooterPlayerController::HandleMenuInput()
{
	if (Btn.Back)
	{
		CloseMenu();
		return;
	}
	const TArray<int32> Tabs = CurrentTabs();
	if ((Btn.TabNext || Btn.TabPrev) && Tabs.Num() > 1)
	{
		int32 Idx = Tabs.Find(MenuTab);
		Idx = (Idx + (Btn.TabNext ? 1 : -1) + Tabs.Num()) % Tabs.Num();
		MenuTab = Tabs[Idx];
		MenuRow = 0;
		ResetPreviews();
		RefreshMenu();
		return;
	}
	bool bChanged = false;
	const int32 NumRows = MenuRows.Num();
	if (Btn.Up && NumRows > 0)
	{
		MenuRow = (MenuRow - 1 + NumRows) % NumRows;
		bChanged = true;
	}
	if (Btn.Down && NumRows > 0)
	{
		MenuRow = (MenuRow + 1) % NumRows;
		bChanged = true;
	}
	if (Btn.Left)
	{
		AdjustRow(-1);
		bChanged = true;
	}
	if (Btn.Right)
	{
		AdjustRow(1);
		bChanged = true;
	}
	if (Btn.Confirm)
	{
		ConfirmRow();
		bChanged = true;
	}
	if (bChanged && Menu != EScooterMenu::None)
	{
		RefreshMenu();
	}
}

void AScooterPlayerController::RefreshMenu()
{
	MenuRows.Reset();
	InfoLines.Reset();
	MenuTabs.Reset();
	static const TCHAR* TabNames[] = { TEXT("FAHRZEUGE"), TEXT("TEILE"), TEXT("AKKU"), TEXT("CONTROLLER"), TEXT("WERKSTATT") };
	const TArray<int32> Tabs = CurrentTabs();
	for (int32 T : Tabs)
	{
		MenuTabs.Add(TabNames[T]);
	}
	MenuTabIndex = FMath::Max(0, Tabs.Find(MenuTab));
	const SS::FVehicleDef& Def = SS::Vehicle(Profile.Current);
	const SS::FControllerDef& Ctrl = SS::Controller(Profile.Setups[Profile.Current].Parts[(int32)SS::EPart::Controller]);
	if (Menu == EScooterMenu::TuningApp)
	{
		MenuTitle = FString::Printf(TEXT("TUNING-APP (%s)  -  %s"), PcTuningApp(Ctrl.Kind), Def.Name);
	}
	else
	{
		MenuTitle = FString::Printf(TEXT("GARAGE  -  %s"), Def.Name);
	}
	switch (MenuTab)
	{
	case PcTabVehicles: BuildVehiclesTab(); break;
	case PcTabParts: BuildPartsTab(); break;
	case PcTabBattery: BuildBatteryTab(); break;
	case PcTabSettings: BuildSettingsTab(); break;
	case PcTabWorkshop: BuildWorkshopTab(); break;
	default: break;
	}
	MenuRow = FMath::Clamp(MenuRow, 0, FMath::Max(0, MenuRows.Num() - 1));
}

void AScooterPlayerController::AddInfo(const FString& Text, const FLinearColor& Color, bool bHeader)
{
	FScooterInfoLine Line;
	Line.Text = Text;
	Line.Color = Color;
	Line.bHeader = bHeader;
	InfoLines.Add(Line);
}

void AScooterPlayerController::AddDynoInfo(const SS::FDynoResult& R, const FString& Title)
{
	AddInfo(Title, PcCyan, true);
	if (R.Failure == SS::EFailure::ControllerOvervoltage)
	{
		AddInfo(TEXT("PENG! \u00dcberspannung - der Controller platzt beim Einschalten."), PcRed);
		return;
	}
	const FString ZeroTo50 = R.ZeroTo50S > 0.f ? FString::Printf(TEXT("%.1f s"), R.ZeroTo50S) : FString(TEXT("-"));
	AddInfo(FString::Printf(TEXT("Topspeed %.0f km/h   |   0-50 km/h: %s   |   Spitze %.1f kW"), R.TopSpeedKmh, *ZeroTo50, R.PeakPowerW / 1000.f));
	AddInfo(FString::Printf(TEXT("Max. Phasenstrom %.0f A   |   Max. Akkustrom %.0f A"), R.PeakPhaseA, R.PeakBattA), PcGrey);
	FString Temps;
	bool bWarn = false;
	for (int32 i = 0; i < SS::NumThermal; ++i)
	{
		const SS::EThermal Part = (SS::EThermal)i;
		const SS::FThermalSpec& Spec = SS::ThermalSpec(Part);
		AddInfo(FString::Printf(TEXT("    %s: %.0f \u00b0C   (kaputt ab %.0f \u00b0C)"), Spec.Name, R.MaxTemp[i], Spec.FailC), PcTempColor(Part, R.MaxTemp[i]));
		bWarn |= R.MaxTemp[i] >= Spec.WarnC;
	}
	if (R.Failure != SS::EFailure::None)
	{
		AddInfo(FString::Printf(TEXT("!! DEFEKT nach %.1f s Vollgas: %s"), R.FailureTimeS, SS::FailureName(R.Failure)), PcRed);
	}
	else if (bWarn)
	{
		AddInfo(TEXT("Grenzwertig: einige Teile werden sehr hei\u00df."), PcOrange);
	}
	if (R.bBmsCut)
	{
		AddInfo(TEXT("!! BMS schaltet ab - Akkustrom \u00fcber dem BMS-Limit."), PcOrange);
	}
	if (R.bWouldWheelie)
	{
		AddInfo(TEXT("Steigt beim Anfahren aufs Hinterrad (Wheelie)!"), PcCyan);
	}
}

void AScooterPlayerController::AddLimitsInfo()
{
	const SS::FVehicleDef& Def = SS::Vehicle(Profile.Current);
	const SS::FVehicleSetup& S = Profile.Setups[Profile.Current];
	const SS::FDerived D = SS::Derive(Def, S);
	const float Phase = D.PhaseA;
	const float BattTotal = D.BattA * (float)Def.NumMotors;
	AddInfo(TEXT("Belastbarkeit deiner Teile (Dauer):"), PcCyan, true);
	AddInfo(FString::Printf(TEXT("Phasenkabel %s: ca. %.0f A   |   eingestellt %.0f A"), SS::OptionName(SS::EPart::PhaseWire, S.Parts[(int32)SS::EPart::PhaseWire]), D.PhaseWireA, Phase),
		PcLoadColor(Phase / D.PhaseWireA));
	AddInfo(FString::Printf(TEXT("Motor: ca. %.0f A   |   Controller: ca. %.0f A"), D.Motor->ContPhaseA, D.Ctrl->ContPhaseA),
		PcLoadColor(Phase / FMath::Min(D.Motor->ContPhaseA, D.Ctrl->ContPhaseA) * 0.6f));
	AddInfo(FString::Printf(TEXT("Akkukabel %s: ca. %.0f A   |   Akkustrom gesamt %.0f A"), SS::OptionName(SS::EPart::BatteryWire, S.Parts[(int32)SS::EPart::BatteryWire]), D.BattWireA, BattTotal),
		PcLoadColor(BattTotal / D.BattWireA));
	AddInfo(FString::Printf(TEXT("Stecker %s: ca. %.0f A"), SS::OptionName(SS::EPart::Connector, S.Parts[(int32)SS::EPart::Connector]), D.ConnectorA),
		PcLoadColor(BattTotal / D.ConnectorA));
	AddInfo(FString::Printf(TEXT("Akku %dS%dP (%s): ca. %.0f A"), S.Series, S.Parallel, SS::Cell(S.Cell).Name, D.Pack.MaxContA),
		PcLoadColor(BattTotal / FMath::Max(D.Pack.MaxContA, 1.f)));
	if (D.Pack.BmsLimitA > 0.f)
	{
		AddInfo(FString::Printf(TEXT("BMS: schaltet ab \u00fcber %.0f A"), D.Pack.BmsLimitA), BattTotal <= D.Pack.BmsLimitA ? PcGreen : PcOrange);
	}
	else
	{
		AddInfo(TEXT("BMS-Bypass: KEIN Schutz - Akkubrand m\u00f6glich!"), PcRed);
	}
	AddInfo(FString::Printf(TEXT("Spannung: Akku voll %.1f V   |   Controller max. %.0f V"), D.Pack.FullV, D.Ctrl->MaxVoltage),
		D.bOvervoltage ? PcRed : PcGreen);
}

void AScooterPlayerController::ShowShopResult(SS::EShopResult Result, const FString& What, float Price)
{
	switch (Result)
	{
	case SS::EShopResult::Bought:
		Notify(FString::Printf(TEXT("Gekauft & eingebaut: %s  (-%s)"), *What, *FormatMoney(Price)), PcGreen, 3.f, false);
		break;
	case SS::EShopResult::Installed:
		Notify(FString::Printf(TEXT("Eingebaut: %s"), *What), PcGreen, 2.5f, false);
		break;
	case SS::EShopResult::NotEnoughMoney:
		Notify(FString::Printf(TEXT("Zu wenig Geld! %s kostet %s"), *What, *FormatMoney(Price)), PcRed, 3.f, false);
		break;
	case SS::EShopResult::NotAvailable:
		Notify(FString::Printf(TEXT("%s passt nicht an dieses Fahrzeug."), *What), PcOrange, 3.f, false);
		break;
	default:
		Notify(FString::Printf(TEXT("%s ist schon verbaut."), *What), PcGrey, 2.f, false);
		break;
	}
}

// ---------------------------------------------------------------------------------------------
// Tabs

void AScooterPlayerController::BuildVehiclesTab()
{
	for (int32 v = 0; v < SS::NumVehicles(); ++v)
	{
		const SS::FVehicleDef& D = SS::Vehicle(v);
		FScooterMenuRow Row;
		Row.Label = D.Name;
		if (Profile.Owned[v])
		{
			Row.Value = v == Profile.Current ? TEXT("AKTIV") : TEXT("in Besitz");
			Row.Color = v == Profile.Current ? PcGreen : FLinearColor::White;
		}
		else
		{
			Row.Value = FormatMoney(D.Price);
			Row.Color = Profile.Money >= D.Price ? FLinearColor::White : PcGrey;
		}
		MenuRows.Add(Row);
	}
	const int32 Sel = FMath::Clamp(MenuRow, 0, SS::NumVehicles() - 1);
	const SS::FVehicleDef& D = SS::Vehicle(Sel);
	const SS::FVehicleSetup Setup = Profile.Owned[Sel] ? Profile.Setups[Sel] : SS::MakeStockSetup(Sel);
	const SS::FDerived Der = SS::Derive(D, Setup);
	AddInfo(D.Name, PcYellow, true);
	AddInfo(D.Description);
	AddInfo(FString::Printf(TEXT("Akku: %.0f V  (%dS%dP %s)  %.1f Ah  /  %.0f Wh"), Der.Pack.NominalV, Setup.Series, Setup.Parallel, SS::Cell(Setup.Cell).Name, Der.Pack.CapacityAh, Der.Pack.EnergyWh));
	AddInfo(FString::Printf(TEXT("Motor: %s%s"), Der.Motor->Name, D.NumMotors > 1 ? TEXT("  (x2 Dual-Motor)") : TEXT("")));
	AddInfo(FString::Printf(TEXT("Controller: %s"), Der.Ctrl->Name));
	AddInfo(FString::Printf(TEXT("Gewicht: %.0f kg  (mit Fahrer %.0f kg)"), D.MassKg, Der.TotalMassKg));
	if (D.StockSpeedLimitKmh > 0.f)
	{
		AddInfo(FString::Printf(TEXT("Ab Werk gedrosselt auf %.0f km/h (im Controller-Men\u00fc abschaltbar)"), D.StockSpeedLimitKmh), PcGrey);
	}
	SS::FVehicleSetup Unlimited = Setup;
	Unlimited.bSpeedLimit = false;
	AddDynoInfo(SS::RunDyno(D, Unlimited, 40.f), Profile.Owned[Sel] ? TEXT("Pr\u00fcfstand (dein Setup, ohne Drossel):") : TEXT("Pr\u00fcfstand (Serie, ohne Drossel):"));
	if (!Profile.Owned[Sel])
	{
		AddInfo(Profile.Money >= D.Price ? FString(TEXT("[Enter] Kaufen")) : FString::Printf(TEXT("Dir fehlen noch %s"), *FormatMoney(D.Price - Profile.Money)),
			Profile.Money >= D.Price ? PcGreen : PcRed, true);
	}
	else if (Sel != Profile.Current)
	{
		AddInfo(TEXT("[Enter] Dieses Fahrzeug fahren"), PcGreen, true);
	}
}

void AScooterPlayerController::BuildPartsTab()
{
	const int32 V = Profile.Current;
	const SS::FVehicleDef& Def = SS::Vehicle(V);
	const SS::FVehicleSetup& Setup = Profile.Setups[V];
	const TArray<SS::EPart> Cats = PartCategoriesForVehicle();
	for (SS::EPart Cat : Cats)
	{
		const int32 c = (int32)Cat;
		const int32 Opt = PartPreview[c];
		FScooterMenuRow Row;
		Row.Label = SS::PartCategoryName(Cat);
		Row.bAdjustable = true;
		Row.Value = SS::OptionName(Cat, Opt);
		if (Opt == Setup.Parts[c])
		{
			Row.Value += TEXT("   [verbaut]");
			Row.Color = PcGreen;
		}
		else if (SS::IsOwned(Setup, Cat, Opt))
		{
			Row.Value += TEXT("   [im Regal]");
			Row.Color = PcCyan;
		}
		else
		{
			const float Price = SS::OptionPrice(Cat, Opt, Def);
			Row.Value += FString::Printf(TEXT("   %s"), *FormatMoney(Price));
			Row.Color = SS::IsOptionAvailable(Cat, Opt, Def) ? (Profile.Money >= Price ? FLinearColor::White : PcGrey) : PcOrange;
		}
		MenuRows.Add(Row);
	}
	if (!Cats.IsValidIndex(MenuRow))
	{
		return;
	}
	const SS::EPart Cat = Cats[MenuRow];
	const int32 c = (int32)Cat;
	const int32 Opt = PartPreview[c];
	AddInfo(FString::Printf(TEXT("%s: %s"), SS::PartCategoryName(Cat), SS::OptionName(Cat, Opt)), PcYellow, true);
	const SS::FPackInfo Pack = SS::GetPackInfo(Setup.Cell, Setup.Series, Setup.Parallel, Setup.Bms);
	switch (Cat)
	{
	case SS::EPart::Controller:
	{
		const SS::FControllerDef& C = SS::Controller(Opt);
		AddInfo(FString::Printf(TEXT("Typ: %s   |   max. Spannung %.0f V"), PcControllerKind(C.Kind), C.MaxVoltage));
		AddInfo(FString::Printf(TEXT("Phasenstrom bis %.0f A   |   Akkustrom bis %.0f A   |   Dauer ca. %.0f A"), C.PeakPhaseA, C.PeakBattA, C.ContPhaseA));
		AddInfo(FString::Printf(TEXT("Feldschw\u00e4chung: %s   |   einstellbar: %s   |   Temperaturschutz: %s"),
			C.MaxFieldWeakA > 0.f ? *FString::Printf(TEXT("bis %.0f A"), C.MaxFieldWeakA) : TEXT("nein"),
			C.bAdjustable ? TEXT("ja") : TEXT("nein"), C.bHasTempProtection ? TEXT("ja") : TEXT("nein")));
		if (Pack.FullV > C.MaxVoltage)
		{
			AddInfo(FString::Printf(TEXT("!! Dein Akku hat voll %.1f V - dieser Controller vertr\u00e4gt nur %.0f V: PENG!"), Pack.FullV, C.MaxVoltage), PcRed);
		}
		if (Def.NumMotors > 1)
		{
			AddInfo(TEXT("Dual-Motor: Preis f\u00fcr zwei Controller."), PcGrey);
		}
		break;
	}
	case SS::EPart::Motor:
	{
		const SS::FMotorDef& M = SS::Motor(Opt);
		AddInfo(FString::Printf(TEXT("Kv %.1f U/min pro Volt   |   Wicklung %.1f mOhm   |   Dauer ca. %.0f A   |   %.1f kg"), M.KvRpmPerV, M.PhaseResistance * 1000.f, M.ContPhaseA, M.MassKg));
		AddInfo(TEXT("Hohes Kv = mehr Topspeed, weniger Drehmoment. Niedriges Kv = mehr Zug."), PcGrey);
		if (Def.NumMotors > 1)
		{
			AddInfo(TEXT("Dual-Motor: Preis f\u00fcr zwei Motoren."), PcGrey);
		}
		break;
	}
	case SS::EPart::PhaseWire:
		AddInfo(FString::Printf(TEXT("Dauerstrom ca. %.0f A pro Phase (Silikonkabel)"), SS::OptionValue(Cat, Opt)));
		AddInfo(FString::Printf(TEXT("Der Querschnitt passt nur bis %s durch die Achse/den Rahmen."), SS::OptionName(Cat, Def.MaxPhaseWire)), PcGrey);
		break;
	case SS::EPart::BatteryWire:
		AddInfo(FString::Printf(TEXT("Dauerstrom ca. %.0f A - tr\u00e4gt den gesamten Akkustrom"), SS::OptionValue(Cat, Opt)));
		break;
	case SS::EPart::Connector:
		AddInfo(FString::Printf(TEXT("Dauerstrom ca. %.0f A - zu kleine Stecker schmelzen!"), SS::OptionValue(Cat, Opt)));
		break;
	case SS::EPart::Tire:
		AddInfo(FString::Printf(TEXT("Grip %.2f (Kurven, Bremsen, Traktion beim Anfahren)"), SS::OptionValue(Cat, Opt)));
		break;
	case SS::EPart::Brake:
		AddInfo(FString::Printf(TEXT("Verz\u00f6gerung bis %.1f m/s\u00b2"), SS::OptionValue(Cat, Opt)));
		break;
	case SS::EPart::Sprocket:
		AddInfo(FString::Printf(TEXT("Mehr Z\u00e4hne = mehr Zug und Wheelies, weniger Topspeed. Serie: %d Z\u00e4hne"), Def.StockSprocketTeeth));
		break;
	default:
		AddInfo(FString::Printf(TEXT("K\u00fchlleistung x%.2f - Teile werden langsamer hei\u00df"), SS::OptionValue(Cat, Opt)));
		break;
	}
	if (!SS::IsOptionAvailable(Cat, Opt, Def))
	{
		AddInfo(TEXT("Passt nicht an dieses Fahrzeug."), PcOrange, true);
		return;
	}
	AddDynoInfo(SS::RunDyno(Def, Setup, 30.f), TEXT("Pr\u00fcfstand JETZT (30 s Vollgas):"));
	if (Opt != Setup.Parts[c])
	{
		SS::FVehicleSetup Preview = Setup;
		Preview.Parts[c] = Opt;
		if (Cat == SS::EPart::Controller)
		{
			SS::ApplyControllerDefaults(Def, Preview);
		}
		AddDynoInfo(SS::RunDyno(Def, Preview, 30.f), TEXT("Pr\u00fcfstand MIT DIESEM TEIL:"));
	}
}

void AScooterPlayerController::BuildBatteryTab()
{
	const int32 V = Profile.Current;
	const SS::FVehicleDef& Def = SS::Vehicle(V);
	const SS::FVehicleSetup& Setup = Profile.Setups[V];
	const SS::FCellType& CellType = SS::Cell(BatCell);
	const SS::FPackInfo Pack = SS::GetPackInfo(BatCell, BatSeries, BatParallel, BatBms);
	const bool bSame = BatCell == Setup.Cell && BatSeries == Setup.Series && BatParallel == Setup.Parallel && BatBms == Setup.Bms;
	const float Price = SS::BatteryPrice(BatCell, BatSeries, BatParallel, BatBms);

	FScooterMenuRow Row;
	Row.bAdjustable = true;
	Row.Label = TEXT("Zelltyp");
	Row.Value = FString::Printf(TEXT("%s   (%.1f Ah, %.0f A, %.1f \u20ac/Zelle)"), CellType.Name, CellType.CapacityAh, CellType.MaxContA, CellType.PricePerCell);
	MenuRows.Add(Row);
	Row.Label = TEXT("Reihe (S)");
	Row.Value = FString::Printf(TEXT("%dS   ->  %.1f V nominal / %.1f V voll"), BatSeries, Pack.NominalV, Pack.FullV);
	MenuRows.Add(Row);
	Row.Label = TEXT("Parallel (P)");
	Row.Value = FString::Printf(TEXT("%dP   ->  %.1f Ah"), BatParallel, Pack.CapacityAh);
	MenuRows.Add(Row);
	Row.Label = TEXT("BMS");
	Row.Value = SS::Bms(BatBms).Name;
	Row.Color = SS::Bms(BatBms).MaxA > 0.f ? FLinearColor::White : PcRed;
	MenuRows.Add(Row);
	Row.Label = TEXT("AKKU BAUEN");
	Row.bAdjustable = false;
	Row.Value = bSame ? FString(TEXT("verbaut")) : FormatMoney(Price);
	Row.Color = bSame ? PcGreen : (Profile.Money >= Price ? PcYellow : PcGrey);
	MenuRows.Add(Row);

	AddInfo(FString::Printf(TEXT("Neuer Akku: %dS%dP = %d Zellen (Platz f\u00fcr max. %d, max. %dS)"), BatSeries, BatParallel, Pack.Cells, Def.MaxCells, Def.MaxSeries), PcYellow, true);
	AddInfo(FString::Printf(TEXT("%.1f V nominal   |   %.1f Ah   |   %.0f Wh   |   %.1f kg"), Pack.NominalV, Pack.CapacityAh, Pack.EnergyWh, Pack.MassKg));
	AddInfo(FString::Printf(TEXT("Innenwiderstand %.0f mOhm   |   Dauerstrom ca. %.0f A"), Pack.ResistanceOhm * 1000.f, Pack.MaxContA));
	const SS::FPackInfo Old = SS::GetPackInfo(Setup.Cell, Setup.Series, Setup.Parallel, Setup.Bms);
	AddInfo(FString::Printf(TEXT("Aktuell: %dS%dP, %.1f V, %.1f Ah, %.0f A"), Setup.Series, Setup.Parallel, Old.NominalV, Old.CapacityAh, Old.MaxContA), PcGrey);

	const SS::FDerived D = SS::Derive(Def, Setup);
	const float BattTotal = D.BattA * (float)Def.NumMotors;
	if (Pack.FullV > D.Ctrl->MaxVoltage)
	{
		AddInfo(FString::Printf(TEXT("!! %.1f V voll ist zu viel f\u00fcr deinen Controller (max. %.0f V): PENG!"), Pack.FullV, D.Ctrl->MaxVoltage), PcRed);
	}
	if (Pack.BmsLimitA <= 0.f)
	{
		AddInfo(TEXT("!! BMS-Bypass: Kein \u00dcberstrom- und Temperaturschutz. Akkubrand m\u00f6glich!"), PcRed);
	}
	else if (Pack.BmsLimitA < BattTotal)
	{
		AddInfo(FString::Printf(TEXT("BMS %.0f A < eingestellter Akkustrom %.0f A: BMS schaltet bei Vollgas ab."), Pack.BmsLimitA, BattTotal), PcOrange);
	}
	if (Pack.MaxContA < BattTotal)
	{
		AddInfo(FString::Printf(TEXT("Die Zellen schaffen nur ca. %.0f A dauerhaft - der Akku wird hei\u00df."), Pack.MaxContA), PcOrange);
	}
	if (!SS::IsBatteryValid(Def, BatCell, BatSeries, BatParallel, BatBms))
	{
		AddInfo(TEXT("Passt so nicht ins Fahrzeug."), PcRed, true);
		return;
	}
	SS::FVehicleSetup Preview = Setup;
	Preview.Cell = BatCell;
	Preview.Series = BatSeries;
	Preview.Parallel = BatParallel;
	Preview.Bms = BatBms;
	AddDynoInfo(SS::RunDyno(Def, Preview, 40.f), bSame ? TEXT("Pr\u00fcfstand (aktueller Akku):") : TEXT("Pr\u00fcfstand MIT DIESEM AKKU:"));
	AddInfo(TEXT("[Links/Rechts] \u00e4ndern   [Enter] Akku bauen lassen"), PcGrey);
}

void AScooterPlayerController::BuildSettingsTab()
{
	const int32 V = Profile.Current;
	const SS::FVehicleDef& Def = SS::Vehicle(V);
	const SS::FVehicleSetup& S = Profile.Setups[V];
	const SS::FControllerDef& C = SS::Controller(S.Parts[(int32)SS::EPart::Controller]);
	const SS::FDerived D = SS::Derive(Def, S);
	const TCHAR* PerCtrl = Def.NumMotors > 1 ? TEXT(" pro Controller") : TEXT("");

	FScooterMenuRow Row;
	Row.bAdjustable = C.bAdjustable;
	Row.Label = TEXT("Phasenstrom");
	Row.Value = C.bAdjustable ? FString::Printf(TEXT("%.0f A   (max. %.0f A)"), D.PhaseA, C.PeakPhaseA) : FString::Printf(TEXT("%.0f A   (Serie, fest)"), D.PhaseA);
	Row.Color = PcLoadColor(D.PhaseA / D.PhaseWireA);
	MenuRows.Add(Row);

	Row.Label = TEXT("Akkustrom");
	Row.Value = C.bAdjustable ? FString::Printf(TEXT("%.0f A%s   (max. %.0f A)"), D.BattA, PerCtrl, C.PeakBattA) : FString::Printf(TEXT("%.0f A%s   (Serie, fest)"), D.BattA, PerCtrl);
	Row.Color = PcLoadColor(D.BattA * (float)Def.NumMotors / FMath::Min(D.BattWireA, D.ConnectorA));
	MenuRows.Add(Row);

	Row.Label = TEXT("Feldschw\u00e4chung");
	Row.bAdjustable = C.bAdjustable && C.MaxFieldWeakA > 0.f;
	Row.Value = Row.bAdjustable ? FString::Printf(TEXT("%.0f A   (max. %.0f A, mehr Topspeed)"), D.FieldWeakA, C.MaxFieldWeakA) : FString(TEXT("nicht verf\u00fcgbar"));
	Row.Color = Row.bAdjustable ? FLinearColor::White : PcGrey;
	MenuRows.Add(Row);

	Row.Label = TEXT("Temperaturschutz");
	Row.bAdjustable = C.bAdjustable && C.bHasTempProtection;
	if (!C.bHasTempProtection)
	{
		Row.Value = TEXT("nicht vorhanden (Billigteil!)");
		Row.Color = PcOrange;
	}
	else
	{
		Row.Value = D.bTempProtect ? FString(TEXT("AN")) : FString(TEXT("AUS - nichts sch\u00fctzt Motor und Controller!"));
		Row.Color = D.bTempProtect ? PcGreen : PcRed;
	}
	MenuRows.Add(Row);

	Row.Label = TEXT("Bremsabschaltung");
	Row.bAdjustable = true;
	Row.Value = S.bBrakeCutoff ? FString(TEXT("AN (Bremse = Motor aus)")) : FString(TEXT("AUS (Gas + Bremse f\u00fcr Wheelie-Kontrolle)"));
	Row.Color = FLinearColor::White;
	MenuRows.Add(Row);

	if (Def.StockSpeedLimitKmh > 0.f)
	{
		Row.Label = FString::Printf(TEXT("Drossel %.0f km/h"), Def.StockSpeedLimitKmh);
		Row.Value = S.bSpeedLimit ? FString(TEXT("AN (legal)")) : FString(TEXT("AUS (illegal - Polizei!)"));
		Row.Color = S.bSpeedLimit ? PcGreen : PcOrange;
		MenuRows.Add(Row);
	}

	AddInfo(FString::Printf(TEXT("%s  (%s)"), C.Name, PcTuningApp(C.Kind)), PcYellow, true);
	if (!C.bAdjustable)
	{
		AddInfo(TEXT("Serien-Controller: Str\u00f6me sind fest. F\u00fcr mehr Power einen VESC oder Fardriver kaufen."), PcGrey);
	}
	else
	{
		AddInfo(TEXT("[Links/Rechts] Wert \u00e4ndern, [Shift] feine Schritte. Zu viel Strom = Teile brennen durch!"), PcGrey);
	}
	AddLimitsInfo();
	AddDynoInfo(SS::RunDyno(Def, S, 60.f), TEXT("Pr\u00fcfstand mit diesen Werten (60 s Vollgas):"));
}

void AScooterPlayerController::BuildWorkshopTab()
{
	const int32 V = Profile.Current;
	const SS::FVehicleSetup& S = Profile.Setups[V];
	const SS::FVehicleState& St = Profile.States[V];
	const bool bBroken = St.Failure != SS::EFailure::None;
	const SS::FPackInfo Pack = SS::GetPackInfo(S.Cell, S.Series, S.Parallel, S.Bms);

	FScooterMenuRow Row;
	Row.Label = TEXT("Zustand");
	Row.Value = SS::FailureName(St.Failure);
	Row.Color = bBroken ? PcRed : PcGreen;
	MenuRows.Add(Row);

	Row.Label = TEXT("Reparieren (Originalteil)");
	Row.Value = bBroken ? FormatMoney(SS::RepairPrice(V, S, St.Failure)) : FString(TEXT("-"));
	Row.Color = bBroken ? PcYellow : PcGrey;
	MenuRows.Add(Row);

	Row.Label = TEXT("Billig-Fix (Serienteil rein)");
	Row.Value = bBroken ? FString(TEXT("gratis")) : FString(TEXT("-"));
	Row.Color = bBroken ? PcYellow : PcGrey;
	MenuRows.Add(Row);

	Row.Label = TEXT("Akku laden");
	Row.Value = FString::Printf(TEXT("%.0f %%  ->  100 %%"), SS::StateOfCharge(Pack, St.AhUsed) * 100.f);
	Row.Color = FLinearColor::White;
	MenuRows.Add(Row);

	Row.Label = TEXT("Controller-Grundeinstellung");
	Row.Value = TEXT("gratis");
	MenuRows.Add(Row);

	Row.Label = TEXT("Alles auf Serie zur\u00fcckbauen");
	Row.Value = TEXT("gratis (Teile bleiben im Regal)");
	MenuRows.Add(Row);

	AddInfo(TEXT("Werkstatt"), PcYellow, true);
	if (bBroken)
	{
		AddInfo(FString::Printf(TEXT("Defekt: %s"), SS::FailureName(St.Failure)), PcRed);
		switch (St.Failure)
		{
		case SS::EFailure::PhaseWire: AddInfo(TEXT("Tipp: dickere Phasenkabel oder weniger Phasenstrom einstellen.")); break;
		case SS::EFailure::BatteryWire: AddInfo(TEXT("Tipp: dickere Akkukabel oder weniger Akkustrom einstellen.")); break;
		case SS::EFailure::Connector: AddInfo(TEXT("Tipp: gr\u00f6\u00dferen Stecker (XT90, AS150, QS8...) einbauen.")); break;
		case SS::EFailure::Controller: AddInfo(TEXT("Tipp: Temperaturschutz an, Controller-K\u00fchlung kaufen oder gr\u00f6\u00dferen Controller.")); break;
		case SS::EFailure::ControllerOvervoltage: AddInfo(TEXT("Tipp: Controller muss mehr Volt k\u00f6nnen als der volle Akku hat (S x 4,2 V).")); break;
		case SS::EFailure::Motor: AddInfo(TEXT("Tipp: Ferrofluid/K\u00fchlung, Temperaturschutz an oder st\u00e4rkeren Motor.")); break;
		case SS::EFailure::BatteryFire: AddInfo(TEXT("Tipp: Nie ohne BMS fahren und gute Zellen mit genug Parallelzellen nehmen.")); break;
		default: break;
		}
	}
	AddInfo(TEXT("Deine Statistik"), PcCyan, true);
	AddInfo(FString::Printf(TEXT("Geld: %s"), *FormatMoney(Profile.Money)));
	AddInfo(FString::Printf(TEXT("Topspeed-Rekord: %.0f km/h"), Profile.BestTopSpeedKmh));
	AddInfo(FString::Printf(TEXT("L\u00e4ngster Wheelie: %.0f m   |   Wheelie gesamt: %.0f m"), Profile.BestWheelieM, Profile.TotalWheelieM));
	int32 Won = 0;
	for (int32 i = 0; i < 32; ++i)
	{
		if (Profile.RacesWon & (1u << (uint32)i))
		{
			++Won;
		}
	}
	AddInfo(FString::Printf(TEXT("Rennen gewonnen: %d   |   Polizei abgeh\u00e4ngt: %d   |   Erwischt: %d"), Won, Profile.Escapes, Profile.Busts));
}

void AScooterPlayerController::AdjustRow(int32 Dir)
{
	const int32 V = Profile.Current;
	const SS::FVehicleDef& Def = SS::Vehicle(V);
	SS::FVehicleSetup& S = Profile.Setups[V];
	switch (MenuTab)
	{
	case PcTabParts:
	{
		const TArray<SS::EPart> Cats = PartCategoriesForVehicle();
		if (!Cats.IsValidIndex(MenuRow))
		{
			return;
		}
		const SS::EPart Cat = Cats[MenuRow];
		const int32 c = (int32)Cat;
		const int32 N = SS::NumOptions(Cat);
		int32 Opt = PartPreview[c];
		for (int32 i = 0; i < N; ++i)
		{
			Opt = (Opt + Dir + N) % N;
			if (SS::IsOptionAvailable(Cat, Opt, Def))
			{
				break;
			}
		}
		PartPreview[c] = Opt;
		break;
	}
	case PcTabBattery:
	{
		switch (MenuRow)
		{
		case 0: BatCell = (BatCell + Dir + SS::NumCells()) % SS::NumCells(); break;
		case 1: BatSeries = FMath::Clamp(BatSeries + Dir, 7, Def.MaxSeries); break;
		case 2: BatParallel = BatParallel + Dir; break;
		case 3: BatBms = (BatBms + Dir + SS::NumBms()) % SS::NumBms(); break;
		default: break;
		}
		BatParallel = FMath::Clamp(BatParallel, 1, FMath::Max(1, Def.MaxCells / FMath::Max(BatSeries, 1)));
		break;
	}
	case PcTabSettings:
	{
		const SS::FControllerDef& C = SS::Controller(S.Parts[(int32)SS::EPart::Controller]);
		const SS::FDerived D = SS::Derive(Def, S);
		switch (MenuRow)
		{
		case 0:
			if (C.bAdjustable)
			{
				const float Step = Btn.Fine ? 1.f : (C.PeakPhaseA > 200.f ? 10.f : 5.f);
				S.PhaseA = FMath::Clamp(D.PhaseA + Step * (float)Dir, 5.f, C.PeakPhaseA);
			}
			break;
		case 1:
			if (C.bAdjustable)
			{
				const float Step = Btn.Fine ? 1.f : (C.PeakBattA > 100.f ? 5.f : 2.f);
				S.BattA = FMath::Clamp(D.BattA + Step * (float)Dir, 3.f, C.PeakBattA);
			}
			break;
		case 2:
			if (C.bAdjustable && C.MaxFieldWeakA > 0.f)
			{
				S.FieldWeakA = FMath::Clamp(D.FieldWeakA + (Btn.Fine ? 1.f : 5.f) * (float)Dir, 0.f, C.MaxFieldWeakA);
			}
			break;
		case 3:
			if (C.bAdjustable && C.bHasTempProtection)
			{
				S.bTempProtect = !S.bTempProtect;
			}
			break;
		case 4:
			S.bBrakeCutoff = !S.bBrakeCutoff;
			break;
		case 5:
			S.bSpeedLimit = !S.bSpeedLimit;
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
}

void AScooterPlayerController::ConfirmRow()
{
	const int32 V = Profile.Current;
	const SS::FVehicleDef& Def = SS::Vehicle(V);
	AScooterPawn* Scooter = GetScooter();
	switch (MenuTab)
	{
	case PcTabVehicles:
	{
		const int32 Sel = FMath::Clamp(MenuRow, 0, SS::NumVehicles() - 1);
		const SS::EShopResult Res = SS::BuyVehicle(Profile, Sel);
		if (Res == SS::EShopResult::Bought || Res == SS::EShopResult::Installed)
		{
			Notify(FString::Printf(TEXT("%s: %s"), Res == SS::EShopResult::Bought ? TEXT("Gekauft") : TEXT("Jetzt f\u00e4hrst du"), SS::Vehicle(Sel).Name), PcGreen, 3.f, true);
			if (Scooter)
			{
				Scooter->RebuildVehicle();
			}
			ResetPreviews();
			SaveProfile();
		}
		else if (Res == SS::EShopResult::NotEnoughMoney)
		{
			ShowShopResult(Res, SS::Vehicle(Sel).Name, SS::Vehicle(Sel).Price);
		}
		break;
	}
	case PcTabParts:
	{
		const TArray<SS::EPart> Cats = PartCategoriesForVehicle();
		if (Cats.IsValidIndex(MenuRow))
		{
			const SS::EPart Cat = Cats[MenuRow];
			const int32 Opt = PartPreview[(int32)Cat];
			const float Price = SS::OptionPrice(Cat, Opt, Def);
			ShowShopResult(SS::BuyOrInstall(Profile, V, Cat, Opt), SS::OptionName(Cat, Opt), Price);
		}
		break;
	}
	case PcTabBattery:
	{
		const float Price = SS::BatteryPrice(BatCell, BatSeries, BatParallel, BatBms);
		const FString What = FString::Printf(TEXT("Akku %dS%dP %s"), BatSeries, BatParallel, SS::Cell(BatCell).Name);
		ShowShopResult(SS::BuildBattery(Profile, V, BatCell, BatSeries, BatParallel, BatBms), What, Price);
		if (Scooter && Profile.States[V].Failure == SS::EFailure::None)
		{
			Scooter->ClearDamageFx();
		}
		break;
	}
	case PcTabSettings:
		if (MenuRow >= 3)
		{
			AdjustRow(1);
		}
		break;
	case PcTabWorkshop:
	{
		SS::FVehicleState& St = Profile.States[V];
		SS::FVehicleSetup& S = Profile.Setups[V];
		switch (MenuRow)
		{
		case 1:
			if (St.Failure != SS::EFailure::None)
			{
				const float Price = SS::RepairPrice(V, S, St.Failure);
				const SS::EShopResult Res = SS::Repair(Profile, V);
				if (Res == SS::EShopResult::Bought)
				{
					Notify(FString::Printf(TEXT("Repariert (-%s)"), *FormatMoney(Price)), PcGreen, 3.f, false);
					if (Scooter) Scooter->ClearDamageFx();
				}
				else
				{
					ShowShopResult(Res, TEXT("Reparatur"), Price);
				}
			}
			break;
		case 2:
			if (St.Failure != SS::EFailure::None)
			{
				SS::CheapFix(Profile, V);
				Notify(TEXT("Serienteil eingebaut. L\u00e4uft wieder!"), PcGreen, 3.f, false);
				if (Scooter) Scooter->ClearDamageFx();
			}
			break;
		case 3:
			St.AhUsed = 0.f;
			Notify(TEXT("Akku voll."), PcGreen, 2.f, false);
			break;
		case 4:
			SS::ApplyControllerDefaults(Def, S);
			Notify(TEXT("Controller auf Grundeinstellung."), PcGreen, 2.f, false);
			break;
		case 5:
			for (int32 c = 0; c < SS::NumPartCategories; ++c)
			{
				S.Parts[c] = SS::StockOption((SS::EPart)c, Def);
			}
			S.Cell = Def.StockCell;
			S.Series = Def.StockSeries;
			S.Parallel = Def.StockParallel;
			S.Bms = Def.StockBms;
			S.bSpeedLimit = Def.StockSpeedLimitKmh > 0.f;
			S.bBrakeCutoff = true;
			SS::ApplyControllerDefaults(Def, S);
			ResetPreviews();
			Notify(TEXT("Alles wieder Serie. Deine Teile liegen im Regal."), PcGreen, 3.f, false);
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
}
