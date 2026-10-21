#include "ScooterSaveGame.h"

namespace SS = ScooterSim;

namespace
{
	constexpr int32 SaveIntsPerVehicle = 2 + SS::NumPartCategories * 2 + 6;
	constexpr int32 SaveFloatsPerVehicle = 4;
}

void UScooterSaveGame::FromProfile(const SS::FProfile& P)
{
	Version = CurrentVersion;
	NumVehicles = SS::NumVehicles();
	Money = P.Money;
	Current = P.Current;
	RacesWon = (int32)P.RacesWon;
	RadarMask = (int32)P.RadarMask;
	Escapes = P.Escapes;
	Busts = P.Busts;
	BestTopSpeed = P.BestTopSpeedKmh;
	BestWheelie = P.BestWheelieM;
	TotalWheelie = P.TotalWheelieM;
	Ints.Reset();
	Floats.Reset();
	for (int32 v = 0; v < NumVehicles; ++v)
	{
		const SS::FVehicleSetup& S = P.Setups[v];
		const SS::FVehicleState& St = P.States[v];
		Ints.Add(P.Owned[v] ? 1 : 0);
		Ints.Add((int32)St.Failure);
		for (int32 c = 0; c < SS::NumPartCategories; ++c)
		{
			Ints.Add(S.Parts[c]);
			Ints.Add((int32)S.Owned[c]);
		}
		Ints.Add(S.Cell);
		Ints.Add(S.Series);
		Ints.Add(S.Parallel);
		Ints.Add(S.Bms);
		Ints.Add((S.bTempProtect ? 1 : 0) | (S.bSpeedLimit ? 2 : 0) | (S.bBrakeCutoff ? 4 : 0));
		Ints.Add(0);
		Floats.Add(S.PhaseA);
		Floats.Add(S.BattA);
		Floats.Add(S.FieldWeakA);
		Floats.Add(St.AhUsed);
	}
}

bool UScooterSaveGame::ToProfile(SS::FProfile& P) const
{
	if (Version != CurrentVersion || NumVehicles != SS::NumVehicles()
		|| Ints.Num() != NumVehicles * SaveIntsPerVehicle || Floats.Num() != NumVehicles * SaveFloatsPerVehicle)
	{
		return false;
	}
	SS::InitProfile(P);
	P.Money = Money;
	P.Current = FMath::Clamp(Current, 0, NumVehicles - 1);
	P.RacesWon = (uint32)RacesWon;
	P.RadarMask = (uint32)RadarMask;
	P.Escapes = Escapes;
	P.Busts = Busts;
	P.BestTopSpeedKmh = BestTopSpeed;
	P.BestWheelieM = BestWheelie;
	P.TotalWheelieM = TotalWheelie;
	int32 I = 0;
	int32 F = 0;
	for (int32 v = 0; v < NumVehicles; ++v)
	{
		SS::FVehicleSetup& S = P.Setups[v];
		SS::FVehicleState& St = P.States[v];
		P.Owned[v] = Ints[I++] != 0;
		St.Failure = (SS::EFailure)FMath::Clamp(Ints[I++], 0, (int32)SS::EFailure::BatteryFire);
		for (int32 c = 0; c < SS::NumPartCategories; ++c)
		{
			S.Parts[c] = FMath::Clamp(Ints[I++], 0, FMath::Max(0, SS::NumOptions((SS::EPart)c) - 1));
			S.Owned[c] = (uint32)Ints[I++];
		}
		S.Cell = FMath::Clamp(Ints[I++], 0, SS::NumCells() - 1);
		S.Series = FMath::Clamp(Ints[I++], 7, 30);
		S.Parallel = FMath::Clamp(Ints[I++], 1, 40);
		S.Bms = FMath::Clamp(Ints[I++], 0, SS::NumBms() - 1);
		const int32 Flags = Ints[I++];
		S.bTempProtect = (Flags & 1) != 0;
		S.bSpeedLimit = (Flags & 2) != 0;
		S.bBrakeCutoff = (Flags & 4) != 0;
		++I;
		S.PhaseA = Floats[F++];
		S.BattA = Floats[F++];
		S.FieldWeakA = Floats[F++];
		St.AhUsed = FMath::Max(0.f, Floats[F++]);
	}
	P.Owned[0] = true;
	if (!P.Owned[P.Current])
	{
		P.Current = 0;
	}
	return true;
}
