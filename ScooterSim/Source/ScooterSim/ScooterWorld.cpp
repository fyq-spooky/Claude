#include "ScooterWorld.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

namespace
{
	constexpr float WorldM = 100.f; // Meter -> cm

	FVector WorldCm(const FVector& M) { return M * WorldM; }
}

AScooterWorld::AScooterWorld()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(SceneRoot);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(SceneRoot);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(FRotator(-38.f, 35.f, 0.f));
	Sun->SetIntensity(8.f);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(SceneRoot);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(SceneRoot);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(SceneRoot);
	Fog->SetFogDensity(0.008f);
}

void AScooterWorld::BeginPlay()
{
	Super::BeginPlay();
	if (Sun)
	{
		Sun->SetAtmosphereSunLight(true);
	}
	Build();
}

void AScooterWorld::Build()
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;

	IsmGround = MakeIsm(EScooterShape::Cube, 0x4E7A3A, true);
	IsmAsphalt = MakeIsm(EScooterShape::Cube, 0x2D2F33, true);
	IsmMarking = MakeIsm(EScooterShape::Cube, 0xE6E6E6, false, false);
	IsmSidewalk = MakeIsm(EScooterShape::Cube, 0x9A9A96, true);
	IsmGrass = MakeIsm(EScooterShape::Cube, 0x5C9642, true);
	IsmPath = MakeIsm(EScooterShape::Cube, 0xC9B98A, false, false);
	IsmWindows = MakeIsm(EScooterShape::Cube, 0x223040, false, false);
	IsmRoof = MakeIsm(EScooterShape::Cube, 0x4A4A4A, false);
	IsmTrunk = MakeIsm(EScooterShape::Cylinder, 0x5A3D2B, true);
	IsmCrown = MakeIsm(EScooterShape::Sphere, 0x2F6B2A, false);
	IsmBollard = MakeIsm(EScooterShape::Cylinder, 0x3C3C3C, true);
	IsmRamp = MakeIsm(EScooterShape::Cube, 0xB07A45, true);
	IsmConcrete = MakeIsm(EScooterShape::Cube, 0xB5B5B0, true);
	IsmBarrier = MakeIsm(EScooterShape::Cube, 0xCFCFC8, true);
	IsmGarage = MakeIsm(EScooterShape::Cube, 0x8C2F2F, true);
	IsmMetal = MakeIsm(EScooterShape::Cube, 0x6F7378, true);
	IsmFountain = MakeIsm(EScooterShape::Cylinder, 0x8FA8C0, true);
	const uint32 BuildingColors[] = { 0xC9BFAF, 0x8E9AA6, 0xB5A48A, 0x7D8C7A, 0xD4C9B6, 0x9C7F6E };
	for (uint32 C : BuildingColors)
	{
		IsmBuildings.Add(MakeIsm(EScooterShape::Cube, C, true));
	}

	BuildGround();
	BuildRoads();
	BuildBlocks();
	BuildGarage();
	BuildHighway();
	BuildScenery();
	BuildGraph();
}

// ---------------------------------------------------------------------------------------------
// Bausteine

UInstancedStaticMeshComponent* AScooterWorld::MakeIsm(EScooterShape Shape, uint32 Color, bool bCollision, bool bShadow)
{
	UInstancedStaticMeshComponent* Comp = ScooterShapes::AddInstanced(this, SceneRoot, Shape, ScooterShapes::Hex(Color), bCollision, bShadow);
	Instanced.Add(Comp);
	return Comp;
}

void AScooterWorld::AddBox(UInstancedStaticMeshComponent* Target, const FVector& CenterM, const FVector& SizeM, const FRotator& Rot)
{
	if (Target)
	{
		Target->AddInstance(ScooterShapes::BoxTransform(WorldCm(CenterM), WorldCm(SizeM), Rot), true);
	}
}

void AScooterWorld::AddRamp(UInstancedStaticMeshComponent* Target, const FVector& StartM, float YawDeg, float LengthM, float HeightM, float WidthM)
{
	// Schraeger Quader: Oberkante laeuft von StartM (Bodenhoehe) bis LengthM weiter auf HeightM.
	const float Angle = FMath::Atan2(HeightM, LengthM);
	const float SlopeLen = FMath::Sqrt(LengthM * LengthM + HeightM * HeightM);
	const float Thick = FMath::Max(0.3f, HeightM * 1.15f);
	const FRotator YawRot(0.f, YawDeg, 0.f);
	const FVector Fwd = YawRot.Vector();
	// Mittelpunkt der Oberkante, dann entlang der Flaechennormale nach unten
	const FVector MidTop = StartM + Fwd * (LengthM * 0.5f) + FVector(0.f, 0.f, HeightM * 0.5f);
	const FVector Normal = Fwd * -FMath::Sin(Angle) + FVector(0.f, 0.f, FMath::Cos(Angle));
	const FVector Center = MidTop - Normal * (Thick * 0.5f);
	AddBox(Target, Center, FVector(SlopeLen, WidthM, Thick), FRotator(FMath::RadiansToDegrees(Angle), YawDeg, 0.f));
	// Stuetzblock unter dem hohen Ende, damit die Rampe massiv wirkt
	const FVector EndBase = StartM + Fwd * LengthM;
	AddBox(Target, EndBase - Fwd * 0.4f + FVector(0.f, 0.f, HeightM * 0.5f - 0.05f), FVector(0.8f, WidthM, HeightM - 0.1f), YawRot);
}

void AScooterWorld::AddBollardLine(const FVector& FromM, const FVector& ToM, float SpacingM)
{
	const FVector Delta = ToM - FromM;
	const float Len = (float)Delta.Size();
	const int32 Count = FMath::Max(1, FMath::FloorToInt(Len / SpacingM));
	for (int32 i = 0; i <= Count; ++i)
	{
		const FVector P = FromM + Delta * ((float)i / (float)Count);
		AddBox(IsmBollard, P + FVector(0.f, 0.f, 0.5f), FVector(0.22f, 0.22f, 1.0f));
	}
}

void AScooterWorld::AddLabel(const FString& Text, const FVector& LocationM, float YawDeg, float SizeM, const FColor& Color)
{
	ScooterShapes::AddText(this, SceneRoot, Text, WorldCm(LocationM), FRotator(0.f, YawDeg, 0.f), SizeM * WorldM, Color);
}

// ---------------------------------------------------------------------------------------------
// Koordinaten

FVector AScooterWorld::GridPoint(int32 I, int32 J) const
{
	return FVector((float)I * BlockM, (float)(J - 4) * BlockM, 0.f) * WorldM;
}

FVector AScooterWorld::HighwayPoint(float DistM) const
{
	return FVector(8.f * BlockM + DistM, 0.f, 0.f) * WorldM;
}

FVector AScooterWorld::GetSpawnLocation() const
{
	return FVector(-24.f, 0.f, 0.f) * WorldM;
}

FVector AScooterWorld::GetRadarGate() const
{
	return HighwayPoint(1800.f);
}

bool AScooterWorld::IsInGarage(const FVector& P) const
{
	return GarageZone.IsInside(FVector2D(P.X, P.Y));
}

bool AScooterWorld::IsInPark(const FVector& P) const
{
	const FVector2D P2(P.X, P.Y);
	for (const FBox2D& Box : ParkZones)
	{
		if (Box.IsInside(P2))
		{
			return true;
		}
	}
	return false;
}

FBox2D AScooterWorld::GetMapBounds() const
{
	return FBox2D(FVector2D(-80.f, -500.f) * WorldM, FVector2D(3950.f, 500.f) * WorldM);
}

// ---------------------------------------------------------------------------------------------
// Bau

void AScooterWorld::BuildGround()
{
	AddBox(IsmGround, FVector(1800.f, 0.f, -0.5f), FVector(9000.f, 7000.f, 1.f));
}

void AScooterWorld::BuildRoads()
{
	const float Span = (float)(GridCount - 1) * BlockM;
	const float HalfRoad = RoadM * 0.5f;
	for (int32 i = 0; i < GridCount; ++i)
	{
		const float X = (float)i * BlockM;
		const float Y = (float)(i - 4) * BlockM;
		AddBox(IsmAsphalt, FVector(X, 0.f, 0.f), FVector(RoadM, Span + RoadM, 0.06f));
		AddBox(IsmAsphalt, FVector(Span * 0.5f, Y, 0.f), FVector(Span + RoadM, RoadM, 0.06f));

		// Mittellinien (gestrichelt), Kreuzungen frei lassen
		for (float D = -Span * 0.5f; D < Span * 0.5f; D += 8.f)
		{
			const float Local = FMath::Fmod(D + Span * 0.5f, BlockM);
			if (Local < HalfRoad + 1.f || Local > BlockM - HalfRoad - 4.f)
			{
				continue;
			}
			AddBox(IsmMarking, FVector(X, D + 1.5f, 0.035f), FVector(0.15f, 3.f, 0.02f));
			AddBox(IsmMarking, FVector(D + Span * 0.5f + 1.5f, Y, 0.035f), FVector(3.f, 0.15f, 0.02f));
		}
	}
}

void AScooterWorld::BuildBlocks()
{
	FRandomStream Rng(1337);
	const float HalfRoad = RoadM * 0.5f;
	const FVector2D CityCenter(4.f * BlockM, 0.f);

	struct FSpecial { int32 I; int32 J; int32 Kind; }; // 1 = Park, 2 = Skatepark
	const FSpecial Specials[] = { { 0, 4, 2 }, { 2, 1, 1 }, { 5, 1, 1 }, { 3, 5, 1 }, { 6, 5, 1 }, { 4, 3, 1 } };

	for (int32 bi = 0; bi < GridCount - 1; ++bi)
	{
		for (int32 bj = 0; bj < GridCount - 1; ++bj)
		{
			const float X0 = (float)bi * BlockM + HalfRoad;
			const float X1 = (float)(bi + 1) * BlockM - HalfRoad;
			const float Y0 = (float)(bj - 4) * BlockM + HalfRoad;
			const float Y1 = (float)(bj - 3) * BlockM - HalfRoad;
			AddBox(IsmSidewalk, FVector((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, 0.f), FVector(X1 - X0, Y1 - Y0, 0.1f));

			const FBox2D Inner(FVector2D(X0 + SidewalkM, Y0 + SidewalkM), FVector2D(X1 - SidewalkM, Y1 - SidewalkM));
			int32 Kind = 0;
			for (const FSpecial& S : Specials)
			{
				if (S.I == bi && S.J == bj)
				{
					Kind = S.Kind;
				}
			}
			if (Kind == 1)
			{
				BuildPark(Inner, Rng);
			}
			else if (Kind == 2)
			{
				BuildSkatepark(Inner);
			}
			else
			{
				const float Dist = (float)FVector2D::Distance(Inner.GetCenter(), CityCenter);
				BuildBuildings(Inner, FMath::Clamp(1.f - Dist / (4.5f * BlockM), 0.f, 1.f), Rng);
			}
		}
	}
}

void AScooterWorld::BuildBuildings(const FBox2D& Area, float CenterFactor, FRandomStream& Rng)
{
	const float Alley = 4.f;
	const float Size = (float)(Area.Max.X - Area.Min.X);
	const float MinH = FMath::Lerp(8.f, 25.f, CenterFactor);
	const float MaxH = FMath::Lerp(22.f, 85.f, CenterFactor);

	auto AddBuilding = [this, &Rng](const FVector2D& Center, float Width, float Depth, float Height)
	{
		UInstancedStaticMeshComponent* Target = IsmBuildings[Rng.RandRange(0, IsmBuildings.Num() - 1)];
		AddBox(Target, FVector((float)Center.X, (float)Center.Y, Height * 0.5f), FVector(Width, Depth, Height));
		AddBox(IsmRoof, FVector((float)Center.X, (float)Center.Y, Height + 0.3f), FVector(Width - 1.f, Depth - 1.f, 0.6f));
		if (Height > 12.f)
		{
			AddBox(IsmRoof, FVector((float)Center.X + Rng.FRandRange(-Width * 0.25f, Width * 0.25f), (float)Center.Y + Rng.FRandRange(-Depth * 0.25f, Depth * 0.25f), Height + 1.5f),
				FVector(4.f, 3.f, 2.f));
		}
		for (float Z = 3.5f; Z < Height - 2.f; Z += 3.6f)
		{
			AddBox(IsmWindows, FVector((float)Center.X, (float)Center.Y - Depth * 0.5f - 0.06f, Z), FVector(Width - 2.f, 0.12f, 1.5f));
			AddBox(IsmWindows, FVector((float)Center.X, (float)Center.Y + Depth * 0.5f + 0.06f, Z), FVector(Width - 2.f, 0.12f, 1.5f));
			AddBox(IsmWindows, FVector((float)Center.X - Width * 0.5f - 0.06f, (float)Center.Y, Z), FVector(0.12f, Depth - 2.f, 1.5f));
			AddBox(IsmWindows, FVector((float)Center.X + Width * 0.5f + 0.06f, (float)Center.Y, Z), FVector(0.12f, Depth - 2.f, 1.5f));
		}
	};

	if (Rng.FRand() < 0.2f)
	{
		// Ein grosser Komplex ohne Gassen
		AddBuilding(Area.GetCenter(), Size - 2.f, Size - 2.f, Rng.FRandRange(MinH, MaxH * 0.6f));
		return;
	}

	const float Lot = (Size - Alley) * 0.5f;
	for (int32 lx = 0; lx < 2; ++lx)
	{
		for (int32 ly = 0; ly < 2; ++ly)
		{
			const FVector2D LotMin(Area.Min.X + (float)lx * (Lot + Alley), Area.Min.Y + (float)ly * (Lot + Alley));
			AddBuilding(LotMin + FVector2D(Lot * 0.5f, Lot * 0.5f), Lot - 1.f, Lot - 1.f, Rng.FRandRange(MinH, MaxH));
		}
	}
	// Poller an den Gassen-Eingaengen: Roller passen durch, Autos nicht
	const float AX = (float)Area.Min.X + Lot + Alley * 0.5f;
	const float AY = (float)Area.Min.Y + Lot + Alley * 0.5f;
	const float Pts[4][2] = { { AX, (float)Area.Min.Y }, { AX, (float)Area.Max.Y }, { (float)Area.Min.X, AY }, { (float)Area.Max.X, AY } };
	for (const auto& Pt : Pts)
	{
		AddBox(IsmBollard, FVector(Pt[0], Pt[1], 0.5f), FVector(0.25f, 0.25f, 1.0f));
	}
}

void AScooterWorld::BuildPark(const FBox2D& Area, FRandomStream& Rng)
{
	ParkZones.Add(FBox2D(Area.Min * WorldM, Area.Max * WorldM));
	const FVector2D C = Area.GetCenter();
	const float Size = (float)(Area.Max.X - Area.Min.X);
	AddBox(IsmGrass, FVector((float)C.X, (float)C.Y, 0.f), FVector(Size, Size, 0.12f));
	AddBox(IsmPath, FVector((float)C.X, (float)C.Y, 0.065f), FVector(Size, 4.f, 0.02f));
	AddBox(IsmPath, FVector((float)C.X, (float)C.Y, 0.065f), FVector(4.f, Size, 0.02f));
	AddBox(IsmFountain, FVector((float)C.X, (float)C.Y, 0.4f), FVector(8.f, 8.f, 0.8f));
	AddBox(IsmFountain, FVector((float)C.X, (float)C.Y, 1.2f), FVector(1.2f, 1.2f, 2.4f));

	// Kleine Schanze auf dem Weg
	AddRamp(IsmRamp, FVector((float)C.X + 14.f, (float)C.Y, 0.06f), 0.f, 3.5f, 0.9f, 3.f);
	AddRamp(IsmRamp, FVector((float)C.X, (float)C.Y - 14.f, 0.06f), -90.f, 3.0f, 0.7f, 3.f);

	for (int32 t = 0; t < 16; ++t)
	{
		const float X = Rng.FRandRange((float)Area.Min.X + 3.f, (float)Area.Max.X - 3.f);
		const float Y = Rng.FRandRange((float)Area.Min.Y + 3.f, (float)Area.Max.Y - 3.f);
		if (FMath::Abs(X - (float)C.X) < 6.f || FMath::Abs(Y - (float)C.Y) < 6.f)
		{
			continue;
		}
		const float H = Rng.FRandRange(3.f, 5.f);
		AddBox(IsmTrunk, FVector(X, Y, H * 0.5f), FVector(0.45f, 0.45f, H));
		const float Crown = Rng.FRandRange(3.5f, 6.f);
		AddBox(IsmCrown, FVector(X, Y, H + Crown * 0.35f), FVector(Crown, Crown, Crown * 0.9f));
	}

	// Pollerreihe rundherum (1,7 m Abstand): Roller rein, Polizeiauto nicht
	const FVector A((float)Area.Min.X, (float)Area.Min.Y, 0.f);
	const FVector B((float)Area.Max.X, (float)Area.Min.Y, 0.f);
	const FVector Cc((float)Area.Max.X, (float)Area.Max.Y, 0.f);
	const FVector D((float)Area.Min.X, (float)Area.Max.Y, 0.f);
	AddBollardLine(A, B, 1.7f);
	AddBollardLine(B, Cc, 1.7f);
	AddBollardLine(Cc, D, 1.7f);
	AddBollardLine(D, A, 1.7f);
}

void AScooterWorld::BuildSkatepark(const FBox2D& Area)
{
	ParkZones.Add(FBox2D(Area.Min * WorldM, Area.Max * WorldM));
	const FVector2D C = Area.GetCenter();
	const float Size = (float)(Area.Max.X - Area.Min.X);
	const float X0 = (float)Area.Min.X;
	const float Y0 = (float)Area.Min.Y;
	AddBox(IsmConcrete, FVector((float)C.X, (float)C.Y, 0.f), FVector(Size, Size, 0.12f));

	// Spur 1: drei Kicker steigender Groesse
	AddRamp(IsmRamp, FVector(X0 + 8.f, Y0 + 12.f, 0.06f), 0.f, 3.f, 0.6f, 3.f);
	AddRamp(IsmRamp, FVector(X0 + 30.f, Y0 + 12.f, 0.06f), 0.f, 4.f, 1.2f, 3.f);
	AddRamp(IsmRamp, FVector(X0 + 55.f, Y0 + 12.f, 0.06f), 0.f, 5.f, 2.0f, 3.5f);

	// Spur 2: Gap-Jump mit Landung
	AddRamp(IsmRamp, FVector(X0 + 14.f, Y0 + 30.f, 0.06f), 0.f, 5.f, 1.6f, 4.f);
	AddRamp(IsmRamp, FVector(X0 + 19.f + 13.f + 8.f, Y0 + 30.f, 0.06f), 180.f, 8.f, 1.6f, 5.f);

	// Spur 3: Quarterpipe-artige Steilrampe am Ende
	AddRamp(IsmRamp, FVector(X0 + Size - 8.f, Y0 + 48.f, 0.06f), 0.f, 4.f, 2.2f, 6.f);

	// Wheelie-Strecke (50 m) mit Markierungen
	const float WY = Y0 + 66.f;
	AddBox(IsmMarking, FVector(X0 + 8.f + 25.f, WY - 2.f, 0.07f), FVector(50.f, 0.2f, 0.02f));
	AddBox(IsmMarking, FVector(X0 + 8.f + 25.f, WY + 2.f, 0.07f), FVector(50.f, 0.2f, 0.02f));
	for (int32 m = 0; m <= 50; m += 10)
	{
		AddBox(IsmMarking, FVector(X0 + 8.f + (float)m, WY, 0.07f), FVector(0.3f, 4.f, 0.02f));
	}
	AddLabel(TEXT("WHEELIE-STRECKE 50 m"), FVector(X0 + 4.f, WY, 2.f), 180.f, 1.f, FColor(255, 220, 60));
	AddLabel(TEXT("SKATEPARK"), FVector((float)C.X, (float)Area.Min.Y + 1.f, 4.f), -90.f, 3.f, FColor(255, 120, 40));

	const FVector A((float)Area.Min.X, (float)Area.Min.Y, 0.f);
	const FVector B((float)Area.Max.X, (float)Area.Min.Y, 0.f);
	const FVector Cc((float)Area.Max.X, (float)Area.Max.Y, 0.f);
	const FVector D((float)Area.Min.X, (float)Area.Max.Y, 0.f);
	AddBollardLine(A, B, 1.7f);
	AddBollardLine(B, Cc, 1.7f);
	AddBollardLine(Cc, D, 1.7f);
	AddBollardLine(D, A, 1.7f);
}

void AScooterWorld::BuildGarage()
{
	const float BackX = -42.f;
	const float FrontX = -14.f;
	const float HalfW = 12.f;
	const float H = 7.f;
	const float MidX = (BackX + FrontX) * 0.5f;
	const float Len = FrontX - BackX;
	GarageZone = FBox2D(FVector2D(BackX + 1.f, -HalfW + 0.5f) * WorldM, FVector2D(FrontX + 1.f, HalfW - 0.5f) * WorldM);

	AddBox(IsmMetal, FVector(MidX, 0.f, 0.f), FVector(Len, HalfW * 2.f, 0.08f));
	AddBox(IsmAsphalt, FVector(-11.f, 0.f, 0.f), FVector(6.f, 20.f, 0.06f));
	AddBox(IsmGarage, FVector(BackX - 0.25f, 0.f, H * 0.5f), FVector(0.5f, HalfW * 2.f + 1.f, H));
	AddBox(IsmGarage, FVector(MidX, -HalfW - 0.25f, H * 0.5f), FVector(Len, 0.5f, H));
	AddBox(IsmGarage, FVector(MidX, HalfW + 0.25f, H * 0.5f), FVector(Len, 0.5f, H));
	AddBox(IsmGarage, FVector(MidX, 0.f, H + 0.25f), FVector(Len + 1.f, HalfW * 2.f + 1.f, 0.5f));
	AddBox(IsmGarage, FVector(FrontX - 0.3f, 0.f, H - 0.6f), FVector(0.6f, HalfW * 2.f + 1.f, 1.2f));
	// Werkbank, Regale, Hebebuehne
	AddBox(IsmMetal, FVector(BackX + 1.f, -6.f, 0.5f), FVector(1.2f, 6.f, 1.0f));
	AddBox(IsmMetal, FVector(BackX + 0.6f, 6.f, 1.8f), FVector(0.6f, 7.f, 3.6f));
	AddBox(IsmMetal, FVector(MidX - 4.f, 0.f, 0.12f), FVector(3.f, 1.2f, 0.16f));
	AddLabel(TEXT("GARAGE"), FVector(FrontX + 0.05f, 0.f, H - 0.6f), 0.f, 1.2f, FColor(255, 200, 40));
	AddLabel(TEXT("[E] Tuning & Shop"), FVector(BackX + 0.1f, 0.f, 3.5f), 0.f, 0.6f, FColor(120, 220, 255));
	ScooterShapes::AddLight(this, SceneRoot, FVector(MidX, -4.f, H - 0.8f) * WorldM, FLinearColor(1.f, 0.85f, 0.65f), 2500.f, 2500.f);
	ScooterShapes::AddLight(this, SceneRoot, FVector(MidX, 4.f, H - 0.8f) * WorldM, FLinearColor(1.f, 0.85f, 0.65f), 2500.f, 2500.f);
}

void AScooterWorld::BuildHighway()
{
	const float StartX = 8.f * BlockM + RoadM * 0.5f;
	const float EndX = 8.f * BlockM + HighwayLengthM;
	const float Len = EndX - StartX;
	const float HalfW = HighwayWidthM * 0.5f;
	AddBox(IsmAsphalt, FVector((StartX + EndX) * 0.5f, 0.f, 0.f), FVector(Len, HighwayWidthM, 0.06f));
	AddBox(IsmMarking, FVector((StartX + EndX) * 0.5f, -HalfW + 0.6f, 0.035f), FVector(Len, 0.2f, 0.02f));
	AddBox(IsmMarking, FVector((StartX + EndX) * 0.5f, HalfW - 0.6f, 0.035f), FVector(Len, 0.2f, 0.02f));
	for (float X = StartX + 5.f; X < EndX; X += 12.f)
	{
		AddBox(IsmMarking, FVector(X, 0.f, 0.035f), FVector(5.f, 0.18f, 0.02f));
	}
	// Leitplanken / Betonwaende
	for (float X = StartX + 25.f; X < EndX - 10.f; X += 50.f)
	{
		AddBox(IsmBarrier, FVector(X, -HalfW - 0.4f, 0.45f), FVector(49.6f, 0.5f, 0.9f));
		AddBox(IsmBarrier, FVector(X, HalfW + 0.4f, 0.45f), FVector(49.6f, 0.5f, 0.9f));
	}
	// Start-/Ziellinien und Entfernungsschilder fuer Dragrennen
	AddBox(IsmMarking, FVector(StartX + 20.f, 0.f, 0.036f), FVector(0.6f, HighwayWidthM - 2.f, 0.02f));
	for (int32 M = 100; M <= 2000; M += 100)
	{
		const float X = StartX + 20.f + (float)M;
		AddLabel(FString::Printf(TEXT("%d m"), M), FVector(X, HalfW + 0.5f, 1.6f), 180.f, 0.8f, FColor::White);
		if (M == 200 || M == 400 || M == 1000 || M == 2000)
		{
			AddBox(IsmMarking, FVector(X, 0.f, 0.036f), FVector(0.6f, HighwayWidthM - 2.f, 0.02f));
		}
	}

	// Radarfalle
	const FVector Gate = GetRadarGate() / WorldM;
	AddBox(IsmMetal, FVector((float)Gate.X, -HalfW - 1.5f, 3.5f), FVector(0.4f, 0.4f, 7.f));
	AddBox(IsmMetal, FVector((float)Gate.X, HalfW + 1.5f, 3.5f), FVector(0.4f, 0.4f, 7.f));
	AddBox(IsmMetal, FVector((float)Gate.X, 0.f, 6.8f), FVector(0.6f, HighwayWidthM + 3.f, 0.6f));
	AddLabel(TEXT("RADAR - TOPSPEED"), FVector((float)Gate.X - 0.35f, 0.f, 6.8f), 180.f, 0.7f, FColor(255, 60, 60));

	// Wendeplatz und Flugfeld mit Mega-Schanze
	const float PlazaX = EndX + 30.f;
	AddBox(IsmAsphalt, FVector(PlazaX, 0.f, 0.f), FVector(90.f, 90.f, 0.06f));
	const float FieldX = PlazaX + 45.f;
	AddBox(IsmAsphalt, FVector(FieldX + 160.f, 0.f, 0.f), FVector(320.f, 70.f, 0.06f));
	AddRamp(IsmRamp, FVector(FieldX + 20.f, 0.f, 0.03f), 0.f, 12.f, 4.f, 8.f);
	AddRamp(IsmRamp, FVector(FieldX + 32.f + 35.f + 20.f, 0.f, 0.03f), 180.f, 20.f, 4.f, 12.f);
	AddBox(IsmBarrier, FVector(FieldX + 318.f, 0.f, 0.6f), FVector(1.f, 70.f, 1.2f));
	AddLabel(TEXT("MEGA-SCHANZE"), FVector(FieldX + 10.f, 0.f, 6.f), 180.f, 2.f, FColor(255, 140, 0));

	SpeedCameras.Add(FVector(3.5f * BlockM, 0.f, 0.f) * WorldM);
	SpeedCameras.Add(FVector(4.f * BlockM, -2.5f * BlockM, 0.f) * WorldM);
	SpeedCameras.Add(HighwayPoint(700.f));
	for (const FVector& Cam : SpeedCameras)
	{
		const FVector M = Cam / WorldM;
		const bool bHighway = M.X > 8.f * BlockM + 10.f;
		const float Side = bHighway ? HalfW + 1.5f : RoadM * 0.5f + 1.5f;
		const bool bAlongX = bHighway || FMath::Abs((float)M.Y) < 1.f;
		const FVector Pole = bAlongX ? FVector((float)M.X, Side, 0.f) : FVector((float)M.X + Side, (float)M.Y, 0.f);
		AddBox(IsmMetal, Pole + FVector(0.f, 0.f, 1.5f), FVector(0.2f, 0.2f, 3.f));
		AddBox(IsmMetal, Pole + FVector(0.f, 0.f, 3.2f), FVector(0.5f, 0.5f, 0.6f));
		AddLabel(TEXT("BLITZER"), Pole + FVector(0.f, 0.f, 3.8f), bAlongX ? 180.f : 90.f, 0.35f, FColor(255, 220, 0));
	}
}

void AScooterWorld::BuildScenery()
{
	UInstancedStaticMeshComponent* Hills = MakeIsm(EScooterShape::Cone, 0x56704A, false, false);
	FRandomStream Rng(42);
	for (int32 i = 0; i < 14; ++i)
	{
		const float Angle = (float)i / 14.f * 2.f * UE_PI;
		const float R = Rng.FRandRange(3200.f, 3800.f);
		const float Size = Rng.FRandRange(700.f, 1400.f);
		AddBox(Hills, FVector(1800.f + FMath::Cos(Angle) * R, FMath::Sin(Angle) * R * 0.8f, Size * 0.3f), FVector(Size, Size, Size * 0.6f));
	}
}

// ---------------------------------------------------------------------------------------------
// Strassengraph

void AScooterWorld::Link(int32 A, int32 B)
{
	NodeLinks[A].AddUnique(B);
	NodeLinks[B].AddUnique(A);
}

void AScooterWorld::BuildGraph()
{
	NodePos.Reset();
	NodeLinks.Reset();
	for (int32 i = 0; i < GridCount; ++i)
	{
		for (int32 j = 0; j < GridCount; ++j)
		{
			NodePos.Add(GridPoint(i, j));
			NodeLinks.AddDefaulted();
		}
	}
	for (int32 i = 0; i < GridCount; ++i)
	{
		for (int32 j = 0; j < GridCount; ++j)
		{
			if (i + 1 < GridCount) Link(GridNode(i, j), GridNode(i + 1, j));
			if (j + 1 < GridCount) Link(GridNode(i, j), GridNode(i, j + 1));
		}
	}
	int32 Prev = GridNode(GridCount - 1, 4);
	for (float D = 200.f; D <= HighwayLengthM + 1.f; D += 200.f)
	{
		NodePos.Add(HighwayPoint(D));
		NodeLinks.AddDefaulted();
		const int32 Idx = NodePos.Num() - 1;
		Link(Prev, Idx);
		Prev = Idx;
	}
}

FVector AScooterWorld::GetNode(int32 Index) const
{
	return NodePos.IsValidIndex(Index) ? NodePos[Index] : FVector::ZeroVector;
}

const TArray<int32>& AScooterWorld::GetLinks(int32 Index) const
{
	static const TArray<int32> Empty;
	return NodeLinks.IsValidIndex(Index) ? NodeLinks[Index] : Empty;
}

int32 AScooterWorld::FindNearestNode(const FVector& P) const
{
	int32 Best = 0;
	double BestD = TNumericLimits<double>::Max();
	for (int32 i = 0; i < NodePos.Num(); ++i)
	{
		const double D = FVector::DistSquared2D(P, NodePos[i]);
		if (D < BestD)
		{
			BestD = D;
			Best = i;
		}
	}
	return Best;
}

int32 AScooterWorld::NextNodeToward(int32 From, int32 Goal) const
{
	if (!NodePos.IsValidIndex(From) || !NodePos.IsValidIndex(Goal) || From == Goal)
	{
		return Goal;
	}
	// Breitensuche vom Ziel aus
	TArray<int32> Dist;
	Dist.Init(-1, NodePos.Num());
	TArray<int32> Queue;
	Queue.Add(Goal);
	Dist[Goal] = 0;
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const int32 N = Queue[Head];
		for (int32 L : NodeLinks[N])
		{
			if (Dist[L] < 0)
			{
				Dist[L] = Dist[N] + 1;
				Queue.Add(L);
			}
		}
	}
	int32 Best = From;
	int32 BestD = MAX_int32;
	for (int32 L : NodeLinks[From])
	{
		if (Dist[L] >= 0 && Dist[L] < BestD)
		{
			BestD = Dist[L];
			Best = L;
		}
	}
	return Best;
}

int32 AScooterWorld::RandomNeighbor(int32 Node, int32 Avoid, FRandomStream& Rng) const
{
	const TArray<int32>& Links = GetLinks(Node);
	if (Links.Num() == 0)
	{
		return Node;
	}
	for (int32 Try = 0; Try < 6; ++Try)
	{
		const int32 Pick = Links[Rng.RandRange(0, Links.Num() - 1)];
		if (Pick != Avoid || Links.Num() == 1)
		{
			return Pick;
		}
	}
	return Links[0];
}
