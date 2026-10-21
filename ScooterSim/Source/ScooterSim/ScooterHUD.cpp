#include "ScooterHUD.h"
#include "ScooterPlayerController.h"
#include "ScooterPawn.h"
#include "ScooterWorld.h"
#include "ScooterSimCore.h"
#include "PoliceManager.h"
#include "PoliceCar.h"
#include "RaceManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"

namespace SS = ScooterSim;

namespace
{
	const FLinearColor HudPanel(0.015f, 0.02f, 0.035f, 0.72f);
	const FLinearColor HudAccent(0.2f, 0.85f, 1.f, 1.f);
	const FLinearColor HudWhite(0.95f, 0.95f, 0.97f, 1.f);
	const FLinearColor HudGrey(0.6f, 0.62f, 0.66f, 1.f);
	const FLinearColor HudGreen(0.3f, 1.f, 0.45f, 1.f);
	const FLinearColor HudOrange(1.f, 0.6f, 0.12f, 1.f);
	const FLinearColor HudRed(1.f, 0.25f, 0.25f, 1.f);
	const FLinearColor HudYellow(1.f, 0.9f, 0.25f, 1.f);
	const FLinearColor HudBlue(0.25f, 0.45f, 1.f, 1.f);

	FLinearColor HudWithAlpha(const FLinearColor& C, float A)
	{
		return FLinearColor(C.R, C.G, C.B, C.A * A);
	}
}

// ---------------------------------------------------------------------------------------------
// Zeichen-Helfer (Koordinaten im 1080p-Design-Raster)

void AScooterHUD::Txt(const FString& Text, float X, float Y, float Px, const FLinearColor& Color, int32 Align)
{
	if (!Canvas || !HudFont || Text.IsEmpty())
	{
		return;
	}
	const float Scale = Px * Ui / BaseCharH;
	float DrawX = X * Ui;
	if (Align != 0)
	{
		const float W = (float)HudFont->GetStringSize(*Text) * Scale;
		DrawX -= Align == 1 ? W * 0.5f : W;
	}
	FCanvasTextItem Item(FVector2D(DrawX, Y * Ui), FText::FromString(Text), HudFont, Color);
	Item.Scale = FVector2D(Scale, Scale);
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f * Color.A));
	Canvas->DrawItem(Item);
}

float AScooterHUD::TxtW(const FString& Text, float Px) const
{
	if (!HudFont)
	{
		return 0.f;
	}
	return (float)HudFont->GetStringSize(*Text) * (Px / BaseCharH);
}

void AScooterHUD::Rect(float X, float Y, float W, float H, const FLinearColor& Color)
{
	DrawRect(Color, X * Ui, Y * Ui, W * Ui, H * Ui);
}

void AScooterHUD::Bar(float X, float Y, float W, float H, float Frac, const FLinearColor& Fill)
{
	Rect(X, Y, W, H, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Rect(X, Y, W * FMath::Clamp(Frac, 0.f, 1.f), H, Fill);
}

void AScooterHUD::Ln(float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Thick)
{
	DrawLine(X1 * Ui, Y1 * Ui, X2 * Ui, Y2 * Ui, Color, Thick * Ui);
}

void AScooterHUD::Panel(float X, float Y, float W, float H)
{
	Rect(X, Y, W, H, HudPanel);
	Rect(X, Y, W, 3.f, HudWithAlpha(HudAccent, 0.8f));
}

bool AScooterHUD::ClipSegment(FVector2D& A, FVector2D& B, const FVector2D& Min, const FVector2D& Max)
{
	// Liang-Barsky
	double T0 = 0.0;
	double T1 = 1.0;
	const FVector2D D = B - A;
	const double P[4] = { -D.X, D.X, -D.Y, D.Y };
	const double Q[4] = { A.X - Min.X, Max.X - A.X, A.Y - Min.Y, Max.Y - A.Y };
	for (int32 i = 0; i < 4; ++i)
	{
		if (FMath::Abs(P[i]) < 1e-9)
		{
			if (Q[i] < 0.0)
			{
				return false;
			}
			continue;
		}
		const double R = Q[i] / P[i];
		if (P[i] < 0.0)
		{
			T0 = FMath::Max(T0, R);
		}
		else
		{
			T1 = FMath::Min(T1, R);
		}
		if (T0 > T1)
		{
			return false;
		}
	}
	const FVector2D Start = A;
	A = Start + D * T0;
	B = Start + D * T1;
	return true;
}

// ---------------------------------------------------------------------------------------------

void AScooterHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	HudFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!HudFont)
	{
		return;
	}
	BaseCharH = FMath::Max(HudFont->GetMaxCharHeight(), 1.f);
	Ui = (float)Canvas->ClipY / 1080.f;
	DesignW = (float)Canvas->ClipX / FMath::Max(Ui, 0.01f);

	AScooterPlayerController* PC = Cast<AScooterPlayerController>(PlayerOwner);
	AScooterPawn* Pawn = PC ? PC->GetScooter() : nullptr;
	if (!PC)
	{
		return;
	}
	if (PC->GetMenu() != EScooterMenu::None)
	{
		DrawMenu(PC);
		DrawNotices(PC);
		return;
	}
	if (Pawn)
	{
		DrawDriving(PC, Pawn);
		DrawMap(PC, Pawn, PC->IsBigMap());
		DrawPoliceAndRace(PC, Pawn);
	}
	DrawNotices(PC);
	if (PC->IsHelpVisible())
	{
		DrawHelp();
	}
}

void AScooterHUD::DrawDriving(AScooterPlayerController* PC, AScooterPawn* Pawn)
{
	const SS::FProfile& Prof = PC->GetProfileConst();
	const int32 V = Pawn->GetVehicleIndex();
	const SS::FVehicleDef& Def = SS::Vehicle(V);
	const SS::FVehicleSetup& Setup = Prof.Setups[V];
	const SS::FVehicleState& St = Prof.States[V];
	const SS::FDerived D = SS::Derive(Def, Setup);
	const SS::FTelemetry& T = Pawn->GetTelemetry();
	const float Kmh = Pawn->GetSpeedKmh();
	const float H = 1080.f;

	// ---- Geld + Fahrzeug (oben links)
	Panel(24.f, 22.f, 470.f, 132.f);
	Txt(AScooterPlayerController::FormatMoney(Prof.Money), 44.f, 30.f, 46.f, HudGreen);
	Txt(Def.Name, 44.f, 84.f, 24.f, HudWhite);
	Txt(FString::Printf(TEXT("%s  |  Rekord %.0f km/h"), D.Ctrl->Name, Prof.BestTopSpeedKmh), 44.f, 116.f, 18.f, HudGrey);

	// ---- Tacho (unten links)
	const float SX = 24.f;
	const float SY = H - 232.f;
	Panel(SX, SY, 340.f, 208.f);
	const FString SpeedStr = FString::Printf(TEXT("%.0f"), Kmh);
	Txt(SpeedStr, SX + 24.f, SY + 6.f, 104.f, Kmh > APoliceManager::LegalKmh ? HudOrange : HudWhite);
	Txt(TEXT("km/h"), SX + 34.f + TxtW(SpeedStr, 104.f), SY + 74.f, 28.f, HudGrey);
	const float SpeedScale = FMath::Max(60.f, FMath::CeilToFloat(FMath::Max(Prof.BestTopSpeedKmh, Kmh) / 20.f) * 20.f);
	Bar(SX + 24.f, SY + 128.f, 292.f, 12.f, Kmh / SpeedScale, Kmh > APoliceManager::LegalKmh ? HudOrange : HudAccent);
	Txt(FString::Printf(TEXT("0 - %.0f km/h"), SpeedScale), SX + 24.f, SY + 144.f, 16.f, HudGrey);
	FString Flags;
	if (T.bSpeedLimited) Flags += TEXT("DROSSEL  ");
	if (T.bVoltageLimited) Flags += TEXT("V-LIMIT  ");
	if (T.bBattLimited) Flags += TEXT("A-LIMIT  ");
	if (T.FwBlend > 0.05f && D.FieldWeakA > 0.f) Flags += TEXT("FW  ");
	if (T.bThermalDerate) Flags += TEXT("TEMP!  ");
	if (T.bWheelspin) Flags += TEXT("DURCHDREHEN  ");
	if (St.BmsCutTimer > 0.f) Flags += TEXT("BMS AUS!  ");
	if (!Setup.bBrakeCutoff) Flags += TEXT("BREMS-CUT AUS  ");
	Txt(Flags, SX + 24.f, SY + 170.f, 18.f, HudYellow);

	// ---- Elektrik
	const float EX = SX + 352.f;
	const float EY = SY;
	Panel(EX, EY, 420.f, 208.f);
	const float Soc = SS::StateOfCharge(D.Pack, St.AhUsed);
	const float Voc = (float)D.Pack.Series * SS::CellOcv(Soc);
	const float BattV = T.BattV > 1.f ? T.BattV : Voc;
	Txt(FString::Printf(TEXT("%.1f kW"), T.PowerW / 1000.f), EX + 20.f, EY + 8.f, 40.f, HudAccent);
	Txt(FString::Printf(TEXT("%.1f V  (Sag %.1f V)"), BattV, FMath::Max(0.f, Voc - BattV)), EX + 400.f, EY + 22.f, 20.f, HudWhite, 2);
	const float PhaseFrac = T.PhaseA / FMath::Max(D.PhaseA, 1.f);
	const float BattTotalSet = D.BattA * (float)Def.NumMotors;
	Txt(FString::Printf(TEXT("Phase  %.0f A"), T.PhaseA), EX + 20.f, EY + 62.f, 20.f, HudWhite);
	Bar(EX + 170.f, EY + 68.f, 230.f, 14.f, PhaseFrac, T.PhaseA > D.PhaseWireA ? HudOrange : HudGreen);
	Txt(FString::Printf(TEXT("Akku   %.0f A"), T.BattA), EX + 20.f, EY + 92.f, 20.f, HudWhite);
	Bar(EX + 170.f, EY + 98.f, 230.f, 14.f, T.BattA / FMath::Max(BattTotalSet, 1.f), T.BattA > D.BattWireA ? HudOrange : HudGreen);
	Txt(FString::Printf(TEXT("Akku   %.0f %%"), Soc * 100.f), EX + 20.f, EY + 122.f, 20.f, Soc < 0.15f ? HudRed : HudWhite);
	Bar(EX + 170.f, EY + 128.f, 230.f, 14.f, Soc, Soc < 0.15f ? HudRed : HudAccent);
	Txt(FString::Printf(TEXT("%dS%dP  |  %.0f Wh  |  %s"), Setup.Series, Setup.Parallel, D.Pack.EnergyWh * Soc, SS::Bms(Setup.Bms).Name), EX + 20.f, EY + 156.f, 16.f, HudGrey);
	Txt(FString::Printf(TEXT("Motor %.0f U/min"), T.MotorRpm), EX + 20.f, EY + 178.f, 16.f, HudGrey);

	// ---- Temperaturen (unten rechts)
	const float TW = 430.f;
	const float TX = DesignW - TW - 24.f;
	const float TY = H - 262.f;
	Panel(TX, TY, TW, 238.f);
	Txt(TEXT("TEMPERATUREN"), TX + 20.f, TY + 8.f, 20.f, HudAccent);
	for (int32 i = 0; i < SS::NumThermal; ++i)
	{
		const SS::FThermalSpec& Spec = SS::ThermalSpec((SS::EThermal)i);
		const float Temp = St.Temp[i];
		const float Frac = (Temp - SS::AmbientTempC) / (Spec.FailC - SS::AmbientTempC);
		const FLinearColor C = Temp >= Spec.FailC ? HudRed : (Temp >= Spec.WarnC ? HudOrange : HudGreen);
		const float RY = TY + 40.f + (float)i * 31.f;
		Txt(Spec.Name, TX + 20.f, RY, 19.f, HudWhite);
		Bar(TX + 160.f, RY + 6.f, 170.f, 13.f, Frac, C);
		Rect(TX + 160.f + 170.f * (Spec.WarnC - SS::AmbientTempC) / (Spec.FailC - SS::AmbientTempC), RY + 4.f, 2.f, 17.f, HudYellow);
		Txt(FString::Printf(TEXT("%.0f \u00b0C"), Temp), TX + TW - 18.f, RY, 19.f, C, 2);
	}
	if (St.Failure != SS::EFailure::None)
	{
		Rect(TX, TY - 46.f, TW, 40.f, FLinearColor(0.5f, 0.02f, 0.02f, 0.85f));
		Txt(SS::FailureName(St.Failure), TX + TW * 0.5f, TY - 40.f, 20.f, HudWhite, 1);
	}

	// ---- Wheelie-Meter / Airtime (rechts Mitte)
	const float Pitch = Pawn->GetWheelieDeg();
	if (Pitch > 1.f || Pawn->IsInWheelie() || Pawn->GetAirTime() > 0.15f)
	{
		const float MX = DesignW - 110.f;
		const float MY = 300.f;
		const float MH = 380.f;
		Panel(MX - 20.f, MY - 60.f, 110.f, MH + 130.f);
		const float Balance = Pawn->GetBalanceDeg();
		const float MaxDeg = Balance + 20.f;
		Rect(MX + 20.f, MY, 30.f, MH, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		const float BalY = MY + MH - MH * Balance / MaxDeg;
		Rect(MX + 20.f, MY, 30.f, BalY - MY, FLinearColor(0.6f, 0.05f, 0.05f, 0.6f));
		Rect(MX + 12.f, BalY - 2.f, 46.f, 4.f, HudYellow);
		const float PY = MY + MH - MH * FMath::Clamp(Pitch / MaxDeg, 0.f, 1.f);
		Rect(MX + 14.f, PY - 5.f, 42.f, 10.f, Pitch > Balance ? HudRed : HudGreen);
		Txt(TEXT("WHEELIE"), MX + 35.f, MY - 50.f, 18.f, HudAccent, 1);
		Txt(FString::Printf(TEXT("%.0f\u00b0"), Pitch), MX + 35.f, MY - 28.f, 22.f, HudWhite, 1);
		if (Pawn->IsInWheelie())
		{
			Txt(FString::Printf(TEXT("%.0f m"), Pawn->GetWheelieDistance()), MX + 35.f, MY + MH + 10.f, 26.f, HudGreen, 1);
		}
		if (Pawn->GetAirTime() > 0.15f)
		{
			Txt(FString::Printf(TEXT("AIR %.1fs"), Pawn->GetAirTime()), MX + 35.f, MY + MH + 40.f, 20.f, HudYellow, 1);
		}
	}

	// ---- Hinweis (unten Mitte)
	const FString& Prompt = PC->GetPrompt();
	if (!Prompt.IsEmpty())
	{
		const float W = FMath::Min(TxtW(Prompt, 24.f) + 50.f, DesignW - 80.f);
		Panel(DesignW * 0.5f - W * 0.5f, H - 320.f, W, 50.f);
		Txt(Prompt, DesignW * 0.5f, H - 310.f, 24.f, HudYellow, 1);
	}
}

void AScooterHUD::DrawMap(AScooterPlayerController* PC, AScooterPawn* Pawn, bool bBig)
{
	AScooterWorld* City = PC->GetCity();
	if (!City)
	{
		return;
	}
	FVector2D Min;
	FVector2D Max;
	FVector2D WorldCenter;
	float K = 0.f; // Pixel pro cm
	const FBox2D Bounds = City->GetMapBounds();
	if (bBig)
	{
		Min = FVector2D(60.f, 200.f);
		Max = FVector2D(DesignW - 60.f, 1080.f - 290.f);
		const FVector2D Size = Bounds.GetSize();
		K = (float)FMath::Min((Max.X - Min.X) / Size.X, (Max.Y - Min.Y) / Size.Y);
		WorldCenter = Bounds.GetCenter();
	}
	else
	{
		Min = FVector2D(DesignW - 340.f, 22.f);
		Max = FVector2D(DesignW - 24.f, 338.f);
		K = (float)(Max.X - Min.X) / 90000.f;
		WorldCenter = FVector2D(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y);
	}
	const FVector2D ScreenCenter = (Min + Max) * 0.5;
	auto ToMap = [&](const FVector& P) { return ScreenCenter + (FVector2D(P.X, P.Y) - WorldCenter) * K; };
	auto Inside = [&](const FVector2D& P) { return P.X >= Min.X && P.X <= Max.X && P.Y >= Min.Y && P.Y <= Max.Y; };

	Rect((float)Min.X, (float)Min.Y, (float)(Max.X - Min.X), (float)(Max.Y - Min.Y), FLinearColor(0.08f, 0.12f, 0.08f, 0.8f));

	for (const FBox2D& Park : City->GetParks())
	{
		FVector2D A = ToMap(FVector(Park.Min.X, Park.Min.Y, 0.0));
		FVector2D B = ToMap(FVector(Park.Max.X, Park.Max.Y, 0.0));
		A.X = FMath::Clamp(A.X, Min.X, Max.X);
		A.Y = FMath::Clamp(A.Y, Min.Y, Max.Y);
		B.X = FMath::Clamp(B.X, Min.X, Max.X);
		B.Y = FMath::Clamp(B.Y, Min.Y, Max.Y);
		if (B.X > A.X && B.Y > A.Y)
		{
			Rect((float)A.X, (float)A.Y, (float)(B.X - A.X), (float)(B.Y - A.Y), FLinearColor(0.2f, 0.5f, 0.2f, 0.9f));
		}
	}
	const float RoadPx = FMath::Max(2.f, AScooterWorld::RoadM * 100.f * K);
	for (int32 i = 0; i < City->NumNodes(); ++i)
	{
		for (int32 L : City->GetLinks(i))
		{
			if (L < i)
			{
				continue;
			}
			FVector2D A = ToMap(City->GetNode(i));
			FVector2D B = ToMap(City->GetNode(L));
			if (ClipSegment(A, B, Min, Max))
			{
				Ln((float)A.X, (float)A.Y, (float)B.X, (float)B.Y, FLinearColor(0.55f, 0.55f, 0.58f, 1.f), RoadPx);
			}
		}
	}
	// Garage
	const FVector2D G = ToMap(City->GetSpawnLocation());
	if (Inside(G))
	{
		Rect((float)G.X - 9.f, (float)G.Y - 9.f, 18.f, 18.f, HudAccent);
		Txt(TEXT("G"), (float)G.X, (float)G.Y - 11.f, 18.f, FLinearColor::Black, 1);
	}
	// Rennen
	if (ARaceManager* Races = PC->GetRaces())
	{
		for (int32 i = 0; i < Races->NumRaces(); ++i)
		{
			const FVector2D M = ToMap(Races->GetMarker(i));
			if (Inside(M))
			{
				Rect((float)M.X - 6.f, (float)M.Y - 6.f, 12.f, 12.f, Races->IsRaceWon(i) ? HudGreen : HudOrange);
			}
		}
		const FRaceHudInfo Info = Races->GetHudInfo();
		if (Info.bHasTarget)
		{
			FVector2D T = ToMap(Info.Target);
			T.X = FMath::Clamp(T.X, Min.X + 6.0, Max.X - 6.0);
			T.Y = FMath::Clamp(T.Y, Min.Y + 6.0, Max.Y - 6.0);
			Rect((float)T.X - 8.f, (float)T.Y - 8.f, 16.f, 16.f, HudYellow);
		}
	}
	// Blitzer
	for (const FVector& Cam : City->GetSpeedCameras())
	{
		const FVector2D C = ToMap(Cam);
		if (Inside(C))
		{
			Rect((float)C.X - 3.f, (float)C.Y - 3.f, 6.f, 6.f, HudWhite);
		}
	}
	// Polizei
	if (APoliceManager* Police = PC->GetPolice())
	{
		const bool bBlink = FMath::Fmod((float)GetWorld()->GetTimeSeconds(), 0.5f) < 0.25f;
		for (APoliceCar* Car : Police->GetCars())
		{
			if (!Car)
			{
				continue;
			}
			const FVector2D P = ToMap(Car->GetActorLocation());
			if (Inside(P))
			{
				const float Sz = Car->IsChasing() ? 12.f : 8.f;
				Rect((float)P.X - Sz * 0.5f, (float)P.Y - Sz * 0.5f, Sz, Sz, Car->IsChasing() ? (bBlink ? HudRed : HudBlue) : HudBlue);
			}
		}
	}
	// Spieler-Pfeil
	const FVector2D P = ToMap(Pawn->GetActorLocation());
	const float YawRad = FMath::DegreesToRadians(Pawn->GetYaw());
	const FVector2D Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad));
	const FVector2D Side(-Fwd.Y, Fwd.X);
	const FVector2D Tip = P + Fwd * 14.0;
	const FVector2D L = P - Fwd * 9.0 + Side * 8.0;
	const FVector2D R = P - Fwd * 9.0 - Side * 8.0;
	Ln((float)Tip.X, (float)Tip.Y, (float)L.X, (float)L.Y, HudYellow, 3.f);
	Ln((float)Tip.X, (float)Tip.Y, (float)R.X, (float)R.Y, HudYellow, 3.f);
	Ln((float)L.X, (float)L.Y, (float)R.X, (float)R.Y, HudYellow, 3.f);

	if (bBig)
	{
		Txt(TEXT("KARTE   [M] schliessen   |   G = Garage   orange = Rennen   gelb = Checkpoint   blau/rot = Polizei   weiss = Blitzer"),
			(float)Min.X, (float)Min.Y - 34.f, 20.f, HudWhite);
	}
}

void AScooterHUD::DrawPoliceAndRace(AScooterPlayerController* PC, AScooterPawn* Pawn)
{
	// ---- Polizei (unter der Minimap)
	APoliceManager* Police = PC->GetPolice();
	if (Police && Police->IsChasing())
	{
		const float X = DesignW - 340.f;
		const float Y = PC->IsBigMap() ? 22.f : 350.f;
		Panel(X, Y, 316.f, 150.f);
		const bool bBlink = FMath::Fmod((float)GetWorld()->GetTimeSeconds(), 0.4f) < 0.2f;
		Txt(TEXT("FAHNDUNG"), X + 16.f, Y + 8.f, 26.f, bBlink ? HudRed : HudBlue);
		for (int32 i = 0; i < 5; ++i)
		{
			Rect(X + 170.f + (float)i * 27.f, Y + 14.f, 22.f, 22.f, i < Police->GetHeat() ? (bBlink ? HudRed : HudBlue) : FLinearColor(0.2f, 0.2f, 0.2f, 0.8f));
		}
		Txt(Police->IsPlayerSeen() ? TEXT("Sichtkontakt!") : TEXT("Versteckt - weiter so"), X + 16.f, Y + 48.f, 20.f, Police->IsPlayerSeen() ? HudOrange : HudGreen);
		Txt(TEXT("Abh\u00e4ngen"), X + 16.f, Y + 80.f, 18.f, HudWhite);
		Bar(X + 130.f, Y + 85.f, 170.f, 12.f, Police->GetEscapeProgress(), HudGreen);
		Txt(TEXT("Geschnappt"), X + 16.f, Y + 108.f, 18.f, HudWhite);
		Bar(X + 130.f, Y + 113.f, 170.f, 12.f, Police->GetBustProgress(), HudRed);
	}

	// ---- Rennen (oben Mitte)
	ARaceManager* Races = PC->GetRaces();
	if (!Races)
	{
		return;
	}
	const FRaceHudInfo Info = Races->GetHudInfo();
	if (!Info.bActive)
	{
		return;
	}
	const float W = 760.f;
	const float X = DesignW * 0.5f - W * 0.5f;
	Panel(X, 22.f, W, 112.f);
	Txt(Info.Title, DesignW * 0.5f, 28.f, 30.f, HudYellow, 1);
	Txt(Info.Line1, DesignW * 0.5f, 68.f, 22.f, HudWhite, 1);
	Txt(Info.Line2, DesignW * 0.5f, 98.f, 18.f, HudGrey, 1);
	if (Info.Countdown > 0.f)
	{
		Txt(FString::Printf(TEXT("%d"), FMath::CeilToInt(Info.Countdown)), DesignW * 0.5f, 380.f, 180.f, HudYellow, 1);
	}
	if (Info.bHasTarget)
	{
		// Pfeil zum naechsten Checkpoint, relativ zur Fahrtrichtung
		const FVector To = Info.Target - Pawn->GetActorLocation();
		const float Rel = FMath::DegreesToRadians((float)FRotator::NormalizeAxis(To.Rotation().Yaw - Pawn->GetYaw()));
		const FVector2D C(DesignW * 0.5f, 190.f);
		const FVector2D Dir(FMath::Sin(Rel), -FMath::Cos(Rel));
		const FVector2D Side(-Dir.Y, Dir.X);
		const FVector2D Tip = C + Dir * 34.0;
		const FVector2D BL = C - Dir * 20.0 + Side * 22.0;
		const FVector2D BR = C - Dir * 20.0 - Side * 22.0;
		Ln((float)Tip.X, (float)Tip.Y, (float)BL.X, (float)BL.Y, HudYellow, 5.f);
		Ln((float)Tip.X, (float)Tip.Y, (float)BR.X, (float)BR.Y, HudYellow, 5.f);
		Ln((float)BL.X, (float)BL.Y, (float)BR.X, (float)BR.Y, HudYellow, 5.f);
		Txt(FString::Printf(TEXT("%.0f m"), (float)To.Size2D() / 100.f), DesignW * 0.5f, 232.f, 22.f, HudYellow, 1);
	}
}

void AScooterHUD::DrawNotices(AScooterPlayerController* PC)
{
	const TArray<FScooterNotice>& Notices = PC->GetNotices();
	const bool bMenu = PC->GetMenu() != EScooterMenu::None;
	float SmallY = bMenu ? 1080.f - 120.f : 170.f;
	for (int32 i = Notices.Num() - 1; i >= 0; --i)
	{
		const FScooterNotice& N = Notices[i];
		const float Alpha = FMath::Clamp(N.TimeLeft / 0.4f, 0.f, 1.f) * FMath::Clamp((N.Duration - N.TimeLeft) / 0.15f + 0.3f, 0.f, 1.f);
		if (N.bBig)
		{
			const float Pop = 1.f + 0.25f * FMath::Clamp(1.f - (N.Duration - N.TimeLeft) / 0.2f, 0.f, 1.f);
			const float W = FMath::Min(TxtW(N.Text, 40.f * Pop) + 60.f, DesignW - 40.f);
			Rect(DesignW * 0.5f - W * 0.5f, 250.f, W, 64.f * Pop, FLinearColor(0.f, 0.f, 0.f, 0.55f * Alpha));
			Txt(N.Text, DesignW * 0.5f, 256.f, 40.f * Pop, HudWithAlpha(N.Color, Alpha), 1);
		}
		else
		{
			const float W = TxtW(N.Text, 21.f) + 30.f;
			const float X = bMenu ? DesignW * 0.5f - W * 0.5f : 24.f;
			Rect(X, SmallY, W, 32.f, FLinearColor(0.f, 0.f, 0.f, 0.55f * Alpha));
			Txt(N.Text, X + 15.f, SmallY + 4.f, 21.f, HudWithAlpha(N.Color, Alpha));
			SmallY += bMenu ? -36.f : 36.f;
		}
	}
}

void AScooterHUD::DrawHelp()
{
	const float X = 24.f;
	const float Y = 420.f;
	const TCHAR* Lines[] = {
		TEXT("W / Pfeil hoch ......... Gas"),
		TEXT("S / Pfeil runter ....... Bremse (im Stand: schieben)"),
		TEXT("A / D ................... Lenken"),
		TEXT("Leertaste ............... Zur\u00fccklehnen = WHEELIE"),
		TEXT("Strg / C ................ Nach vorne lehnen"),
		TEXT("E ....................... Garage / Rennen starten"),
		TEXT("T ....................... Tuning-App (im Stand)"),
		TEXT("R ....................... Zur\u00fccksetzen / Abschleppen"),
		TEXT("M ....................... Karte     H ... Hilfe"),
		TEXT("Gamepad: RT Gas, LT Bremse, rechter Stick = Lehnen"),
		TEXT("Tipp: \u00dcber 30 km/h jagt dich die Polizei!"),
	};
	const int32 Num = (int32)(sizeof(Lines) / sizeof(Lines[0]));
	Panel(X, Y, 560.f, 50.f + (float)Num * 28.f);
	Txt(TEXT("STEUERUNG  [H]"), X + 18.f, Y + 10.f, 22.f, HudAccent);
	for (int32 i = 0; i < Num; ++i)
	{
		Txt(Lines[i], X + 18.f, Y + 44.f + (float)i * 28.f, 19.f, i >= Num - 2 ? HudYellow : HudWhite);
	}
}

void AScooterHUD::DrawMenu(AScooterPlayerController* PC)
{
	const float H = 1080.f;
	Rect(0.f, 0.f, DesignW, H, FLinearColor(0.01f, 0.012f, 0.02f, 0.86f));
	Txt(PC->GetMenuTitle(), 60.f, 30.f, 38.f, HudWhite);
	Txt(AScooterPlayerController::FormatMoney(PC->GetProfileConst().Money), DesignW - 60.f, 30.f, 38.f, HudGreen, 2);

	const TArray<FString>& Tabs = PC->GetMenuTabs();
	float TabX = 60.f;
	for (int32 i = 0; i < Tabs.Num(); ++i)
	{
		const bool bSel = i == PC->GetMenuTabIndex();
		const float W = TxtW(Tabs[i], 22.f) + 44.f;
		Rect(TabX, 92.f, W, 42.f, bSel ? HudWithAlpha(HudAccent, 0.9f) : FLinearColor(1.f, 1.f, 1.f, 0.08f));
		Txt(Tabs[i], TabX + 22.f, 100.f, 22.f, bSel ? FLinearColor::Black : HudGrey);
		TabX += W + 10.f;
	}

	// Liste links
	const float LX = 60.f;
	const float LY = 156.f;
	const float LW = FMath::Min(860.f, DesignW * 0.46f);
	const float RowH = 42.f;
	const TArray<FScooterMenuRow>& Rows = PC->GetMenuRows();
	const int32 MaxRows = 19;
	const int32 First = FMath::Clamp(PC->GetMenuRow() - MaxRows / 2, 0, FMath::Max(0, Rows.Num() - MaxRows));
	Rect(LX, LY, LW, (float)FMath::Min(Rows.Num(), MaxRows) * RowH + 10.f, HudPanel);
	for (int32 i = First; i < Rows.Num() && i < First + MaxRows; ++i)
	{
		const FScooterMenuRow& Row = Rows[i];
		const float Y = LY + 5.f + (float)(i - First) * RowH;
		const bool bSel = i == PC->GetMenuRow();
		if (bSel)
		{
			Rect(LX + 4.f, Y, LW - 8.f, RowH - 4.f, HudWithAlpha(HudAccent, 0.22f));
			Rect(LX + 4.f, Y, 5.f, RowH - 4.f, HudAccent);
		}
		Txt(Row.Label, LX + 22.f, Y + 7.f, 21.f, bSel ? HudWhite : HudGrey);
		FString Value = Row.Value;
		if (bSel && Row.bAdjustable)
		{
			Value = FString(TEXT("<  ")) + Value + TEXT("  >");
		}
		Txt(Value, LX + LW - 18.f, Y + 8.f, 19.f, Row.Color, 2);
	}

	// Info rechts
	const float IX = LX + LW + 30.f;
	const float IW = DesignW - IX - 60.f;
	Rect(IX, LY, IW, H - LY - 90.f, HudPanel);
	float Y = LY + 14.f;
	for (const FScooterInfoLine& Line : PC->GetInfoLines())
	{
		if (Y > H - 120.f)
		{
			break;
		}
		if (Line.bHeader)
		{
			Y += 6.f;
			Txt(Line.Text, IX + 20.f, Y, 23.f, Line.Color);
			Y += 32.f;
		}
		else
		{
			Txt(Line.Text, IX + 20.f, Y, 18.f, Line.Color);
			Y += 25.f;
		}
	}

	const FString Hint = PC->GetMenu() == EScooterMenu::TuningApp
		? FString(TEXT("[W/S] Auswahl   [A/D] Wert \u00e4ndern   [Shift] fein   [Backspace / B] Schlie\u00dfen"))
		: FString(TEXT("[W/S] Auswahl   [A/D] Teil/Wert \u00e4ndern   [Enter] Kaufen / Einbauen   [Tab / Q] Reiter   [Backspace / B] Schlie\u00dfen"));
	Txt(Hint, 60.f, H - 60.f, 20.f, HudGrey);
}
