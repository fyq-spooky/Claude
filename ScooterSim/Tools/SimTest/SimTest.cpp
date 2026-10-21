// Standalone-Test fuer den Simulationskern (ohne Unreal).
// Bauen: siehe Tools/SimTest/build.sh
#include "../../Source/ScooterSim/ScooterSimCore.h"
#include <cstdio>
#include <cwchar>
#include <clocale>
#include <cmath>
#include <cstdlib>

using namespace ScooterSim;

static int GFailures = 0;

// Wandelt TCHAR-Namen fuer printf in UTF-8 um (Ringpuffer, reicht fuer ein printf)
static const char* U8(const TCHAR* W)
{
	static char Buf[8][256];
	static int Slot = 0;
	char* Out = Buf[Slot];
	Slot = (Slot + 1) % 8;
	std::wcstombs(Out, W, 255);
	Out[255] = 0;
	return Out;
}

static void Expect(bool bCond, const char* What)
{
	if (!bCond)
	{
		std::printf("  FEHLER: %s\n", What);
		++GFailures;
	}
	else
	{
		std::printf("  ok: %s\n", What);
	}
}

static void PrintDyno(const FVehicleDef& Def, const FVehicleSetup& S, const FDynoResult& R)
{
	const FDerived D = Derive(Def, S);
	std::printf("%-34s %3dS%2dP %5.1fV | %6.1f km/h | 0-50 %5.1fs | %5.1f kW | Ph %5.0fA Ak %5.0fA | ",
		U8(Def.Name), S.Series, S.Parallel, D.Pack.NominalV, R.TopSpeedKmh, R.ZeroTo50S, R.PeakPowerW / 1000.f, R.PeakPhaseA, R.PeakBattA);
	for (int32 i = 0; i < NumThermal; ++i)
	{
		std::printf("%3.0f ", R.MaxTemp[i]);
	}
	if (R.Failure != EFailure::None)
	{
		std::printf("| DEFEKT nach %.1fs: %s", R.FailureTimeS, U8(FailureName(R.Failure)));
	}
	if (R.bBmsCut) std::printf(" | BMS-Cut");
	if (R.bWouldWheelie) std::printf(" | Wheelie!");
	std::printf("\n");
}

// Dauerlast (z.B. Vollgas bergauf): Geschwindigkeit wird festgehalten.
static EFailure SustainedLoad(int32 VehicleIndex, const FVehicleSetup& S, float SpeedMs, float Seconds, float& OutBattTemp, bool& bOutBmsCut)
{
	const FVehicleDef& Def = Vehicle(VehicleIndex);
	const FDerived D = Derive(Def, S);
	FVehicleState St = FreshState();
	FRideState Ride = FRideState();
	bOutBmsCut = false;
	for (int32 Step = 0; Step < (int32)(Seconds / 0.01f); ++Step)
	{
		Ride.Speed = SpeedMs;
		Ride.Pitch = 0.f;
		Ride.PitchRate = 0.f;
		FDriveInputs In;
		In.Dt = 0.01f;
		In.Throttle = 1.f;
		In.Brake = 0.f;
		In.Lean = 0.f;
		In.SlopeSin = 0.f;
		In.SlopeCos = 1.f;
		In.bGrounded = true;
		FTelemetry Out;
		StepVehicle(Def, D, St, Ride, In, Out);
		bOutBmsCut |= Out.bBmsCut;
		if (St.Failure != EFailure::None)
		{
			break;
		}
	}
	OutBattTemp = St.Temp[(int32)EThermal::Battery];
	return St.Failure;
}

// Versucht von 0 aus einen Wheelie und haelt ihn mit einem einfachen Regler (Gas + Lehnen).
static float WheelieTest(int32 VehicleIndex, const FVehicleSetup& S, bool bHoldFullThrottle, float& OutMaxPitchDeg, bool& bOutLoop)
{
	const FVehicleDef& Def = Vehicle(VehicleIndex);
	const FDerived D = Derive(Def, S);
	FVehicleState St = FreshState();
	FRideState Ride = FRideState();
	const float Target = BalancePointRad(Def, 0.6f) * 0.6f;
	float WheelieTime = 0.f;
	OutMaxPitchDeg = 0.f;
	bOutLoop = false;
	for (int32 Step = 0; Step < 1500; ++Step)
	{
		FDriveInputs In;
		In.Dt = 0.01f;
		In.SlopeSin = 0.f;
		In.SlopeCos = 1.f;
		In.bGrounded = true;
		In.Brake = 0.f;
		if (bHoldFullThrottle)
		{
			In.Throttle = 1.f;
			In.Lean = 1.f;
		}
		else
		{
			const float Err = Target - Ride.Pitch;
			In.Throttle = Clampf(0.75f + Err * 3.f - Ride.PitchRate * 0.8f, 0.f, 1.f);
			In.Lean = Clampf(Err * 6.f - Ride.PitchRate * 1.5f, -1.f, 1.f);
			if (Ride.Pitch > Target * 1.4f)
			{
				In.Brake = 0.5f;
			}
		}
		FTelemetry Out;
		StepVehicle(Def, D, St, Ride, In, Out);
		OutMaxPitchDeg = Maxf(OutMaxPitchDeg, Ride.Pitch * 57.2958f);
		if (Ride.Pitch > 0.05f)
		{
			WheelieTime += In.Dt;
		}
		if (Out.bLoopOut)
		{
			bOutLoop = true;
			break;
		}
	}
	return WheelieTime;
}

int main()
{
	std::setlocale(LC_ALL, "");
	std::printf("=== Serienfahrzeuge auf dem Pruefstand (60 s Vollgas) ===\n");
	std::printf("Temperaturen: Controller Motor Akku Phasenkabel Akkukabel Stecker\n");
	for (int32 v = 0; v < NumVehicles(); ++v)
	{
		FVehicleSetup S = MakeStockSetup(v);
		S.bSpeedLimit = false;
		const FDynoResult R = RunDyno(Vehicle(v), S, 60.f);
		PrintDyno(Vehicle(v), S, R);
		if (R.Failure != EFailure::None || R.bBmsCut)
		{
			Expect(false, "Serienfahrzeug muss 60 s Vollgas ohne Defekt/BMS-Cut schaffen");
		}
	}

	std::printf("\n=== Drossel ===\n");
	{
		FVehicleSetup S = MakeStockSetup(0);
		const FDynoResult R = RunDyno(Vehicle(0), S, 30.f);
		PrintDyno(Vehicle(0), S, R);
		Expect(R.TopSpeedKmh < 21.f, "Flitzer gedrosselt <= 20 km/h");
	}

	std::printf("\n=== Kukirin G4 + Fardriver ND72450 ===\n");
	{
		FProfile P;
		InitProfile(P);
		P.Money = 100000.f;
		const int32 G4 = 7;
		BuyVehicle(P, G4);
		FVehicleSetup& S = P.Setups[G4];
		S.bSpeedLimit = false;
		Expect(BuyOrInstall(P, G4, EPart::Controller, 21) == EShopResult::Bought, "Fardriver gekauft");
		std::printf("  Werkstatt-Grundeinstellung: Phase %.0fA, Akku %.0fA\n", S.PhaseA, S.BattA);
		FDynoResult R = RunDyno(Vehicle(G4), S, 60.f);
		PrintDyno(Vehicle(G4), S, R);
		Expect(R.Failure == EFailure::None, "Grundeinstellung ist sicher");

		// Klassiker: Controller + BMS aufgeruestet, aber Serien-Akkukabel (12 AWG) und XT90 behalten
		BuildBattery(P, G4, 4, 16, 7, 4);
		S.PhaseA = 300.f;
		S.BattA = 150.f;
		R = RunDyno(Vehicle(G4), S, 60.f);
		PrintDyno(Vehicle(G4), S, R);
		Expect(R.Failure == EFailure::PhaseWire || R.Failure == EFailure::BatteryWire || R.Failure == EFailure::Connector,
			"150A Akkustrom ueber 12 AWG / XT90 -> Kabel oder Stecker brennt durch");
		Expect(R.FailureTimeS > 0.f && R.FailureTimeS < 6.f, "brennt innerhalb weniger Sekunden");

		// Richtig aufgebaut: dicke Kabel, guter Akku, BMS, Stecker
		BuyOrInstall(P, G4, EPart::PhaseWire, 4);
		BuyOrInstall(P, G4, EPart::BatteryWire, 6);
		BuyOrInstall(P, G4, EPart::Connector, 4);
		BuildBattery(P, G4, 4, 20, 6, 4);
		S.PhaseA = 160.f;
		S.BattA = 110.f;
		R = RunDyno(Vehicle(G4), S, 60.f);
		PrintDyno(Vehicle(G4), S, R);
		Expect(R.TopSpeedKmh > 85.f, "getunter G4 deutlich schneller als Serie");

		S.FieldWeakA = 120.f;
		R = RunDyno(Vehicle(G4), S, 60.f);
		PrintDyno(Vehicle(G4), S, R);

		// Ueberspannung: 24S Akku an 90V Fardriver
		BuildBattery(P, G4, 4, 24, 5, 4);
		Expect(Derive(Vehicle(G4), S).bOvervoltage == false || S.Series > 20, "24S passt nicht in G4 (MaxSeries)");
	}

	std::printf("\n=== Ueberspannung ===\n");
	{
		FProfile P;
		InitProfile(P);
		P.Money = 100000.f;
		const int32 V = 3; // G2 Pro, Serien-Controller 60V max
		BuyVehicle(P, V);
		Expect(BuildBattery(P, V, 1, 16, 4, 2) == EShopResult::Bought, "16S Akku gebaut");
		const FDynoResult R = RunDyno(Vehicle(V), P.Setups[V], 5.f);
		PrintDyno(Vehicle(V), P.Setups[V], R);
		Expect(R.Failure == EFailure::ControllerOvervoltage, "16S (67V) an 60V-Controller -> Peng");
	}

	std::printf("\n=== Dauerlast: BMS-Bypass -> Akkubrand ===\n");
	{
		FProfile P;
		InitProfile(P);
		P.Money = 100000.f;
		const int32 V = 7;
		BuyVehicle(P, V);
		FVehicleSetup& S = P.Setups[V];
		S.bSpeedLimit = false;
		BuyOrInstall(P, V, EPart::Controller, 21);
		BuyOrInstall(P, V, EPart::PhaseWire, 5);
		BuyOrInstall(P, V, EPart::BatteryWire, 6);
		BuyOrInstall(P, V, EPart::Connector, 4);
		S.PhaseA = 160.f;
		S.BattA = 100.f;
		float BattT = 0.f;
		bool bCut = false;
		BuildBattery(P, V, 1, 16, 3, 2); // 35E 16S3P (max 24A), BMS 50A
		EFailure F = SustainedLoad(V, S, 12.f, 120.f, BattT, bCut);
		std::printf("  mit BMS 50A:  Defekt=%s, Akku %.0f C, BMS-Cut %d\n", U8(FailureName(F)), BattT, bCut ? 1 : 0);
		Expect(F == EFailure::None && bCut, "BMS schuetzt den Akku (schaltet ab)");
		BuildBattery(P, V, 1, 16, 3, 7); // gleicher Akku, Bypass
		F = SustainedLoad(V, S, 12.f, 120.f, BattT, bCut);
		std::printf("  mit Bypass:   Defekt=%s, Akku %.0f C\n", U8(FailureName(F)), BattT);
		Expect(F == EFailure::BatteryFire, "Bypass + 100A aus 24A-Akku -> Akkubrand");
	}

	std::printf("\n=== Wheelies ===\n");
	{
		const int32 Tests[] = { 0, 3, 7, 10, 11, 13 };
		for (int32 v : Tests)
		{
			FVehicleSetup S = MakeStockSetup(v);
			S.bSpeedLimit = false;
			float MaxDeg = 0.f;
			bool bLoop = false;
			const float Held = WheelieTest(v, S, false, MaxDeg, bLoop);
			float MaxDegFull = 0.f;
			bool bLoopFull = false;
			WheelieTest(v, S, true, MaxDegFull, bLoopFull);
			std::printf("%-34s Regler: %5.1fs Wheelie, max %4.1f Grad, Loop %d | Vollgas+Zuruecklehnen: max %4.1f Grad, Loop %d | BP %4.1f\n",
				U8(Vehicle(v).Name), Held, MaxDeg, bLoop ? 1 : 0, MaxDegFull, bLoopFull ? 1 : 0, BalancePointRad(Vehicle(v), 0.f) * 57.3f);
		}
		{
			FProfile P;
			InitProfile(P);
			P.Money = 100000.f;
			BuyVehicle(P, 7);
			BuyOrInstall(P, 7, EPart::Motor, 17);
			BuyOrInstall(P, 7, EPart::Controller, 21);
			BuyOrInstall(P, 7, EPart::PhaseWire, 5);
			BuyOrInstall(P, 7, EPart::BatteryWire, 6);
			BuyOrInstall(P, 7, EPart::Connector, 4);
			BuildBattery(P, 7, 4, 20, 6, 4);
			P.Setups[7].PhaseA = 260.f;
			P.Setups[7].BattA = 130.f;
			P.Setups[7].bSpeedLimit = false;
			float MaxDeg = 0.f;
			bool bLoop = false;
			const float Held = WheelieTest(7, P.Setups[7], false, MaxDeg, bLoop);
			std::printf("Kukirin G4 getunt (Torque-Motor, 260A) Regler: %5.1fs Wheelie, max %4.1f Grad\n", Held, MaxDeg);
			Expect(Held > 3.f, "stark getunter Roller kann Wheelie");
		}
		FVehicleSetup Flitzer = MakeStockSetup(0);
		float MaxDeg = 0.f;
		bool bLoop = false;
		Expect(WheelieTest(0, Flitzer, true, MaxDeg, bLoop) < 0.1f, "Billigroller schafft keinen Wheelie");
		FVehicleSetup Lbx = MakeStockSetup(10);
		Expect(WheelieTest(10, Lbx, false, MaxDeg, bLoop) > 5.f, "Sur-Ron kann Wheelie halten");
		WheelieTest(10, Lbx, true, MaxDeg, bLoop);
		Expect(bLoop, "Sur-Ron Vollgas + Zuruecklehnen -> Ueberschlag");
	}

	std::printf("\n=== Wirtschaft ===\n");
	{
		FProfile P;
		InitProfile(P);
		Expect(P.Money == 300.f && P.Owned[0], "Start: 300 Euro + Flitzer");
		Expect(BuyVehicle(P, 9) == EShopResult::NotEnoughMoney, "Dualtron zu teuer");
		Expect(BuyOrInstall(P, 0, EPart::Controller, 15) == EShopResult::Bought, "Billig-Sinus-Controller fuer den Flitzer");
		Expect(BuyOrInstall(P, 0, EPart::Controller, 0) == EShopResult::Installed, "Serien-Controller zurueck (gratis)");
		Expect(BuyOrInstall(P, 0, EPart::Controller, 15) == EShopResult::Installed, "Sinus-Controller wieder rein (gehoert dir)");
		Expect(BuyOrInstall(P, 0, EPart::Motor, 23) == EShopResult::NotAvailable, "Mittelmotor passt nicht in Roller");
		Expect(BuyOrInstall(P, 0, EPart::PhaseWire, 8) == EShopResult::NotAvailable, "2 AWG passt nicht durch die Achse");
		P.States[0].Failure = EFailure::PhaseWire;
		OnFailure(P, 0, EFailure::PhaseWire);
		const float Before = P.Money;
		CheapFix(P, 0);
		Expect(P.Money == Before && P.States[0].Failure == EFailure::None, "Billig-Fix ist gratis");
		P.States[0].Failure = EFailure::Controller;
		OnFailure(P, 0, EFailure::Controller);
		Expect(!IsOwned(P.Setups[0], EPart::Controller, 15), "abgerauchter Controller ist weg");
	}

	std::printf("\n%s (%d Fehler)\n", GFailures == 0 ? "ALLE TESTS OK" : "TESTS FEHLGESCHLAGEN", GFailures);
	return GFailures == 0 ? 0 : 1;
}
