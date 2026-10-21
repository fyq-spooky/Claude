// Prueft die Boden-/Sprunglogik des Pawns (nachgebaut ohne Unreal) auf einem 2D-Hoehenprofil:
// Anlauf -> Kicker -> Luecke -> Landerampe -> flach. Gleiche Schwellwerte wie AScooterPawn.
#include "../../Source/ScooterSim/ScooterSimCore.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ScooterSim;

// Hoehenprofil in cm. Kicker 5 m lang auf 1,6 m (wie im Skatepark), Landerampe 8 m.
static float SurfaceAt(float XCm)
{
	const float X = XCm / 100.f;
	if (X >= 20.f && X <= 25.f) return (X - 20.f) / 5.f * 160.f;   // Kicker hoch
	if (X > 38.f && X <= 46.f) return (46.f - X) / 8.f * 160.f;    // Landerampe runter
	return 0.f;
}

// Wie AScooterPawn::ProbeGround: Strahl von RefZ+120 cm nach unten. Startet er "in" der Rampe, sieht er nur den Boden.
static bool Probe(float XCm, float RefZ, float& OutZ)
{
	const float Start = RefZ + 120.f;
	const float S = SurfaceAt(XCm);
	OutZ = (Start >= S) ? S : 0.f;
	return OutZ <= Start && OutZ >= RefZ - 400.f;
}

int main()
{
	const int32 V = 10; // Sur-Ron
	const FVehicleDef& Def = Vehicle(V);
	FVehicleSetup Setup = MakeStockSetup(V);
	const FDerived D = Derive(Def, Setup);
	FVehicleState St = FreshState();
	FRideState Ride = FRideState();
	Ride.Speed = 16.f;

	const float HalfWB = Def.WheelbaseM * 50.f;
	float X = 500.f; // Fahrzeugmitte, cm
	float FrontZ = 0.f, RearZ = 0.f, Slope = 0.f;
	bool bAir = false;
	float AirVX = 0.f, AirVZ = 0.f, AirZ = 0.f, AirTime = 0.f, MaxAirZ = 0.f;
	int32 Takeoffs = 0, Landings = 0;
	float TakeoffX = 0.f, LandX = 0.f, LandVInto = 0.f, LandRel = 0.f;
	const float Dt = 1.f / 60.f;

	for (int32 Frame = 0; Frame < 60 * 8 && X < 7000.f; ++Frame)
	{
		FDriveInputs In;
		In.Throttle = 0.35f;
		In.Brake = 0.f;
		In.Lean = 0.f;
		In.SlopeSin = std::sin(Slope);
		In.SlopeCos = std::cos(Slope);
		In.bGrounded = !bAir;
		In.Dt = Dt / 4.f;
		In.AirPathAngle = std::atan2(AirVZ, AirVX);
		for (int32 i = 0; i < 4; ++i)
		{
			FTelemetry Out;
			StepVehicle(Def, D, St, Ride, In, Out);
		}
		if (!bAir)
		{
			const float Moved = Ride.Speed * std::cos(Slope) * 100.f * Dt;
			X += Moved;
			const float Rise = Moved * std::tan(Slope);
			const float ExpF = FrontZ + Rise, ExpR = RearZ + Rise;
			float Zf = ExpF, Zr = ExpR;
			const bool bF = Probe(X + HalfWB, ExpF, Zf) && Zf < ExpF + 35.f;
			const bool bR = Probe(X - HalfWB, ExpR, Zr) && Zr < ExpR + 35.f;
			const bool bDropF = !bF || (ExpF - Zf > 25.f);
			const bool bDropR = !bR || (ExpR - Zr > 25.f);
			if (bDropF && bDropR)
			{
				bAir = true;
				++Takeoffs;
				TakeoffX = X;
				AirVX = Ride.Speed * std::cos(Slope);
				AirVZ = Ride.Speed * std::sin(Slope);
				AirZ = (ExpF + ExpR) * 0.5f;
				Ride.AirPitch = Slope + Ride.Pitch;
				Ride.AirPitchRate = 0.f;
				AirTime = 0.f;
				continue;
			}
			if (bDropF) Zf = ExpF;
			if (bDropR) Zr = ExpR;
			FrontZ = Zf;
			RearZ = Zr;
			Slope = std::atan2(Zf - Zr, HalfWB * 2.f);
		}
		else
		{
			AirTime += Dt;
			AirVZ -= Gravity * Dt;
			X += AirVX * 100.f * Dt;
			AirZ += AirVZ * 100.f * Dt;
			MaxAirZ = std::max(MaxAirZ, AirZ);
			const float FrontWheel = AirZ + std::sin(Ride.AirPitch) * HalfWB;
			const float RearWheel = AirZ - std::sin(Ride.AirPitch) * HalfWB;
			float Zf = 0.f, Zr = 0.f;
			const bool bF = Probe(X + HalfWB, FrontWheel, Zf);
			const bool bR = Probe(X - HalfWB, RearWheel, Zr);
			if ((bF && FrontWheel <= Zf + 3.f) || (bR && RearWheel <= Zr + 3.f))
			{
				const float S = std::atan2(Zf - Zr, HalfWB * 2.f);
				const float VAlong = AirVX * std::cos(S) + AirVZ * std::sin(S);
				LandVInto = -(-AirVX * std::sin(S) + AirVZ * std::cos(S));
				LandRel = Ride.AirPitch - S;
				LandX = X;
				++Landings;
				bAir = false;
				FrontZ = Zf;
				RearZ = Zr;
				Slope = S;
				Ride.Speed = VAlong;
				Ride.Pitch = 0.f;
			}
		}
	}
	std::printf("Absprung bei %.1f m, Landung bei %.1f m, Flughoehe max %.2f m, Flugzeit %.2f s\n",
		TakeoffX / 100.f, LandX / 100.f, MaxAirZ / 100.f, AirTime);
	std::printf("Aufprall senkrecht zur Flaeche %.1f m/s (Crash ab 11), Winkel relativ %.1f Grad, Speed danach %.1f km/h\n",
		LandVInto, LandRel * 57.3f, Ride.Speed * 3.6f);
	std::printf("Absprung-Ereignisse %d, Landungen %d, Ende bei %.1f m\n", Takeoffs, Landings, X / 100.f);
	const bool bOk = Takeoffs == 1 && Landings == 1 && TakeoffX > 2400.f && TakeoffX < 2600.f && LandVInto < 11.f && LandRel > -0.6f && LandRel < 0.3f;
	std::printf("%s\n", bOk ? "SPRUNG-TEST OK" : "SPRUNG-TEST FEHLGESCHLAGEN");
	return bOk ? 0 : 1;
}
