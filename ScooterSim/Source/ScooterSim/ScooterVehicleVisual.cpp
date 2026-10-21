#include "ScooterVehicleVisual.h"
#include "ScooterShapes.h"
#include "ScooterSimCore.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	struct FVisualBuildCtx
	{
		AActor* Owner = nullptr;
		USceneComponent* Body = nullptr;
		USceneComponent* Steer = nullptr;
		USceneComponent* Rider = nullptr;
		float Lr = 0.5f;
		TArray<TObjectPtr<USceneComponent>>* Created = nullptr;

		// Fahrzeugkoordinaten (x ab Hinterachse, z ab Boden, Meter) -> cm relativ zum LeanPivot
		FVector P(float X, float Y, float Z) const { return FVector((X - Lr) * 100.f, Y * 100.f, Z * 100.f); }
		// Koordinaten relativ zur Vorderachse (fuer Teile an der Lenkung)
		static FVector F(float DX, float Y, float Z) { return FVector(DX * 100.f, Y * 100.f, Z * 100.f); }
	};

	void VisTrack(FVisualBuildCtx& C, USceneComponent* Comp)
	{
		if (Comp && C.Created)
		{
			C.Created->Add(Comp);
		}
	}

	void VisBox(FVisualBuildCtx& C, USceneComponent* Parent, const FVector& LocCm, const FVector& SizeM, const FLinearColor& Color,
		const FRotator& Rot = FRotator::ZeroRotator)
	{
		VisTrack(C, ScooterShapes::AddShape(C.Owner, Parent, EScooterShape::Cube, LocCm, SizeM * 100.f, Color, Rot));
	}

	void VisSphere(FVisualBuildCtx& C, USceneComponent* Parent, const FVector& LocCm, float DiameterM, const FLinearColor& Color)
	{
		const float D = DiameterM * 100.f;
		VisTrack(C, ScooterShapes::AddShape(C.Owner, Parent, EScooterShape::Sphere, LocCm, FVector(D, D, D), Color));
	}

	// Zylinder quer zur Fahrtrichtung (Achse entlang Y), z.B. Lenker
	void VisCrossCylinder(FVisualBuildCtx& C, USceneComponent* Parent, const FVector& LocCm, float DiameterM, float LengthM, const FLinearColor& Color)
	{
		const float D = DiameterM * 100.f;
		VisTrack(C, ScooterShapes::AddShape(C.Owner, Parent, EScooterShape::Cylinder, LocCm, FVector(D, D, LengthM * 100.f), Color, FRotator(0.f, 0.f, 90.f)));
	}

	void VisBeam(FVisualBuildCtx& C, USceneComponent* Parent, const FVector& FromCm, const FVector& ToCm, float ThickM, const FLinearColor& Color)
	{
		VisTrack(C, ScooterShapes::AddBeam(C.Owner, Parent, FromCm, ToCm, ThickM * 100.f, Color));
	}

	USceneComponent* VisWheel(FVisualBuildCtx& C, USceneComponent* Parent, const FVector& AxleCm, float R, float Width, const FLinearColor& Rim, bool bSpokes)
	{
		USceneComponent* Spin = ScooterShapes::AddPivot(C.Owner, Parent, AxleCm);
		VisTrack(C, Spin);
		const FLinearColor Tire = ScooterShapes::Hex(0x151515);
		VisCrossCylinder(C, Spin, FVector::ZeroVector, R * 2.f, Width, Tire);
		VisCrossCylinder(C, Spin, FVector::ZeroVector, R * 2.f * (bSpokes ? 0.88f : 0.62f), Width * (bSpokes ? 0.6f : 1.15f), Rim);
		// Speichen-Kreuz, damit man das Drehen sieht
		const float SpokeLen = R * 2.f * (bSpokes ? 0.86f : 0.58f);
		VisBox(C, Spin, FVector::ZeroVector, FVector(SpokeLen, Width * 1.25f, 0.025f), ScooterShapes::Hex(0x3A3A3A));
		VisBox(C, Spin, FVector::ZeroVector, FVector(SpokeLen, Width * 1.25f, 0.025f), ScooterShapes::Hex(0x3A3A3A), FRotator(90.f, 0.f, 0.f));
		return Spin;
	}

	struct FRiderColors
	{
		FLinearColor Jacket;
		FLinearColor Helmet;
		FLinearColor Pants = ScooterShapes::Hex(0x22252B);
		FLinearColor Skin = ScooterShapes::Hex(0xD9A57E);
		FLinearColor Shoes = ScooterShapes::Hex(0x101010);
	};

	void VisRiderUpper(FVisualBuildCtx& C, const FVector& HipCm, const FVector& ShoulderCm, const FVector& HandLCm, const FVector& HandRCm, const FRiderColors& Col)
	{
		USceneComponent* R = C.Rider;
		const FVector Mid = (HipCm + ShoulderCm) * 0.5f;
		const FVector Up = ShoulderCm - HipCm;
		const float LeanDeg = -FMath::RadiansToDegrees(FMath::Atan2((float)Up.X, (float)Up.Z));
		VisBox(C, R, Mid, FVector(0.24f, 0.38f, (float)Up.Size() / 100.f + 0.06f), Col.Jacket, FRotator(LeanDeg, 0.f, 0.f));
		const FVector Head = ShoulderCm + FVector(5.f, 0.f, 19.f);
		VisSphere(C, R, Head, 0.21f, Col.Skin);
		VisSphere(C, R, Head + FVector(-1.f, 0.f, 3.f), 0.27f, Col.Helmet);
		VisBox(C, R, Head + FVector(10.f, 0.f, 1.f), FVector(0.06f, 0.18f, 0.08f), ScooterShapes::Hex(0x0A0A0A));
		const FVector ShL = ShoulderCm + FVector(0.f, -19.f, -4.f);
		const FVector ShR = ShoulderCm + FVector(0.f, 19.f, -4.f);
		VisBeam(C, R, ShL, HandLCm, 0.085f, Col.Jacket);
		VisBeam(C, R, ShR, HandRCm, 0.085f, Col.Jacket);
		VisSphere(C, R, HandLCm, 0.09f, Col.Shoes);
		VisSphere(C, R, HandRCm, 0.09f, Col.Shoes);
	}

	void VisStandingRider(FVisualBuildCtx& C, float DeckTop, float FootRearX, float FootFrontX, float HandX, float HandHalfY, float HandZ, const FRiderColors& Col)
	{
		const float HipX = (FootRearX + FootFrontX) * 0.5f - 0.03f;
		const float HipZ = DeckTop + 0.9f;
		const FVector FootR = C.P(FootRearX, -0.05f, DeckTop + 0.04f);
		const FVector FootF = C.P(FootFrontX, 0.05f, DeckTop + 0.04f);
		const FVector HipL = C.P(HipX, -0.1f, HipZ);
		const FVector HipR = C.P(HipX, 0.1f, HipZ);
		VisBeam(C, C.Rider, FootR, HipL, 0.13f, Col.Pants);
		VisBeam(C, C.Rider, FootF, HipR, 0.13f, Col.Pants);
		VisBox(C, C.Rider, FootR, FVector(0.26f, 0.1f, 0.08f), Col.Shoes);
		VisBox(C, C.Rider, FootF, FVector(0.26f, 0.1f, 0.08f), Col.Shoes);
		const FVector Hip = C.P(HipX, 0.f, HipZ);
		const FVector Shoulder = C.P(HipX + 0.13f, 0.f, HipZ + 0.55f);
		VisRiderUpper(C, Hip, Shoulder, C.P(HandX, -HandHalfY, HandZ), C.P(HandX, HandHalfY, HandZ), Col);
	}

	void VisSeatedRider(FVisualBuildCtx& C, const FVector& HipM, const FVector& KneeM, const FVector& FootM, const FVector& ShoulderM,
		float HandX, float HandHalfY, float HandZ, const FRiderColors& Col)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float S = (float)Side;
			const FVector Hip = C.P((float)HipM.X, S * 0.1f, (float)HipM.Z);
			const FVector Knee = C.P((float)KneeM.X, S * (float)KneeM.Y, (float)KneeM.Z);
			const FVector Foot = C.P((float)FootM.X, S * (float)FootM.Y, (float)FootM.Z);
			VisBeam(C, C.Rider, Hip, Knee, 0.14f, Col.Pants);
			VisBeam(C, C.Rider, Knee, Foot, 0.12f, Col.Pants);
			VisBox(C, C.Rider, Foot + FVector(4.f, 0.f, -3.f), FVector(0.25f, 0.1f, 0.08f), Col.Shoes);
		}
		VisRiderUpper(C, C.P((float)HipM.X, 0.f, (float)HipM.Z), C.P((float)ShoulderM.X, 0.f, (float)ShoulderM.Z),
			C.P(HandX, -HandHalfY, HandZ), C.P(HandX, HandHalfY, HandZ), Col);
	}

	void VisBuildScooter(FVisualBuildCtx& C, const ScooterSim::FVehicleDef& Def, const FRiderColors& RC, FScooterVisual& Out)
	{
		using namespace ScooterShapes;
		const float WB = Def.WheelbaseM;
		const float R = Def.WheelRadiusM;
		const FLinearColor Primary = Hex(Def.ColorPrimary);
		const FLinearColor Secondary = Hex(Def.ColorSecondary);
		const FLinearColor Dark = Hex(0x1C1C1C);
		const FLinearColor Metal = Hex(0x9A9A9A);

		float DeckW = 0.19f, DeckT = 0.07f, WheelW = 0.075f, BarW = 0.58f, StemD = 0.045f;
		if (Def.Style == ScooterSim::EVisualStyle::CheapScooter)
		{
			DeckW = 0.14f; DeckT = 0.045f; WheelW = 0.055f; BarW = 0.44f; StemD = 0.035f;
		}
		else if (Def.Style == ScooterSim::EVisualStyle::HyperScooter)
		{
			DeckW = 0.25f; DeckT = 0.11f; WheelW = 0.11f; BarW = 0.72f; StemD = 0.06f;
		}
		const float DeckZ = R * 0.9f + DeckT * 0.5f;
		const float DeckTop = DeckZ + DeckT * 0.5f;
		const float DeckLen = WB - R * 1.5f;

		VisBox(C, C.Body, C.P(WB * 0.5f, 0.f, DeckZ), FVector(DeckLen, DeckW, DeckT), Primary);
		VisBox(C, C.Body, C.P(WB * 0.5f, 0.f, DeckTop + 0.003f), FVector(DeckLen - 0.06f, DeckW - 0.02f, 0.008f), Dark);
		VisBox(C, C.Body, C.P(R * 0.45f, 0.f, R * 0.95f), FVector(R * 1.0f, WheelW + 0.04f, 0.04f), Primary);
		VisBox(C, C.Body, C.P(0.02f, 0.f, R * 2.f + 0.025f), FVector(R * 2.0f, WheelW + 0.03f, 0.012f), Secondary, FRotator(-6.f, 0.f, 0.f));
		if (Def.Style == ScooterSim::EVisualStyle::HyperScooter)
		{
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				VisBox(C, C.Body, C.P(WB * 0.5f, (float)Side * (DeckW * 0.5f + 0.004f), DeckZ - DeckT * 0.25f), FVector(DeckLen - 0.1f, 0.008f, 0.025f), Secondary);
			}
			VisBeam(C, C.Body, C.P(R * 0.3f, 0.f, R * 1.3f), C.P(R * 1.6f, 0.f, DeckZ), 0.05f, Secondary);
		}
		Out.RearSpin = VisWheel(C, C.Body, C.P(0.f, 0.f, R), R, WheelW, Metal, false);

		// Lenkung
		C.Steer = ScooterShapes::AddPivot(C.Owner, C.Body, C.P(WB, 0.f, 0.f));
		VisTrack(C, C.Steer);
		Out.SteerPivot = C.Steer;
		Out.FrontSpin = VisWheel(C, C.Steer, FVisualBuildCtx::F(0.f, 0.f, R), R, WheelW, Metal, false);
		VisBox(C, C.Steer, FVisualBuildCtx::F(0.f, 0.f, R * 2.f + 0.03f), FVector(R * 1.8f, WheelW + 0.03f, 0.012f), Secondary);

		const float StemTopZ = 1.06f;
		const FVector StemBottom = FVisualBuildCtx::F(-0.03f, 0.f, R + 0.06f);
		const FVector StemTop = FVisualBuildCtx::F(-0.14f, 0.f, StemTopZ);
		if (Def.Style == ScooterSim::EVisualStyle::HyperScooter)
		{
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const FVector Off(0.f, (float)Side * 6.f, 0.f);
				VisBeam(C, C.Steer, StemBottom + Off, StemTop + Off, StemD, Dark);
			}
		}
		else
		{
			VisBeam(C, C.Steer, StemBottom, StemTop, StemD, Dark);
		}
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float Y = (float)Side * (WheelW * 0.5f + 0.018f);
			VisBeam(C, C.Steer, FVisualBuildCtx::F(0.f, Y, R), FVisualBuildCtx::F(-0.035f, Y, R + 0.2f), StemD * 0.7f, Metal);
		}
		VisCrossCylinder(C, C.Steer, StemTop, 0.032f, BarW, Dark);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			VisCrossCylinder(C, C.Steer, StemTop + FVector(0.f, (float)Side * (BarW * 50.f - 5.f), 0.f), 0.042f, 0.11f, Hex(0x050505));
		}
		VisBox(C, C.Steer, StemTop + FVector(2.f, 0.f, 3.f), FVector(0.06f, 0.1f, 0.03f), Hex(0x050505));
		const FVector LightPos = FMath::Lerp(StemBottom, StemTop, 0.55f) + FVector(5.f, 0.f, 0.f);
		VisSphere(C, C.Steer, LightPos, Def.Style == ScooterSim::EVisualStyle::HyperScooter ? 0.12f : 0.07f, Hex(0xFFF3C4));

		VisStandingRider(C, DeckTop, WB * 0.32f, WB * 0.56f, WB - 0.14f, BarW * 0.5f - 0.06f, StemTopZ, RC);

		Out.DeckPoint = C.P(WB * 0.5f, 0.f, DeckZ);
		Out.MotorPoint = C.P(0.f, 0.f, R);
		Out.ControllerPoint = C.P(WB * 0.65f, 0.f, DeckZ);
	}

	void VisBuildMoto(FVisualBuildCtx& C, const ScooterSim::FVehicleDef& Def, const FRiderColors& RC, FScooterVisual& Out)
	{
		using namespace ScooterShapes;
		const float WB = Def.WheelbaseM;
		const float R = Def.WheelRadiusM;
		const FLinearColor Primary = Hex(Def.ColorPrimary);
		const FLinearColor Secondary = Hex(Def.ColorSecondary);
		const FLinearColor Frame = Hex(0x3A3A3A);
		const FLinearColor Metal = Hex(0xB8B8B8);
		const FLinearColor Black = Hex(0x111111);
		const float WheelW = 0.11f;

		Out.RearSpin = VisWheel(C, C.Body, C.P(0.f, 0.f, R), R, WheelW, Hex(0x202020), false);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float Y = (float)Side * 0.075f;
			VisBeam(C, C.Body, C.P(0.f, Y, R), C.P(0.5f, Y, 0.48f), 0.05f, Frame);
		}
		VisBeam(C, C.Body, C.P(0.26f, 0.f, 0.56f), C.P(0.56f, 0.f, 0.86f), 0.055f, Hex(0xE0A800));
		VisBeam(C, C.Body, C.P(0.52f, 0.f, 0.5f), C.P(WB - 0.22f, 0.f, 0.98f), 0.08f, Frame);
		VisBeam(C, C.Body, C.P(0.3f, 0.f, 0.82f), C.P(WB - 0.25f, 0.f, 1.0f), 0.06f, Frame);
		VisBox(C, C.Body, C.P(0.68f, 0.f, 0.66f), FVector(0.4f, 0.2f, 0.34f), Primary);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			VisBox(C, C.Body, C.P(0.66f, (float)Side * 0.115f, 0.66f), FVector(0.42f, 0.02f, 0.3f), Secondary);
		}
		VisCrossCylinder(C, C.Body, C.P(0.5f, 0.f, 0.42f), 0.2f, 0.17f, Hex(0x2A2A2A));
		VisBox(C, C.Body, C.P(0.55f, 0.f, 0.9f), FVector(0.62f, 0.22f, 0.08f), Black);
		VisBox(C, C.Body, C.P(0.1f, 0.f, 0.85f), FVector(0.45f, 0.16f, 0.04f), Primary, FRotator(8.f, 0.f, 0.f));
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			VisBox(C, C.Body, C.P(0.47f, (float)Side * 0.17f, 0.36f), FVector(0.06f, 0.08f, 0.03f), Metal);
		}

		C.Steer = ScooterShapes::AddPivot(C.Owner, C.Body, C.P(WB, 0.f, 0.f));
		VisTrack(C, C.Steer);
		Out.SteerPivot = C.Steer;
		Out.FrontSpin = VisWheel(C, C.Steer, FVisualBuildCtx::F(0.f, 0.f, R), R, WheelW * 0.9f, Hex(0x202020), false);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float Y = (float)Side * 0.085f;
			VisBeam(C, C.Steer, FVisualBuildCtx::F(0.f, Y, R), FVisualBuildCtx::F(-0.24f, Y, 1.0f), 0.05f, Hex(0xC9A227));
		}
		VisBox(C, C.Steer, FVisualBuildCtx::F(-0.24f, 0.f, 1.0f), FVector(0.08f, 0.22f, 0.04f), Frame);
		VisCrossCylinder(C, C.Steer, FVisualBuildCtx::F(-0.26f, 0.f, 1.08f), 0.03f, 0.78f, Black);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			VisCrossCylinder(C, C.Steer, FVisualBuildCtx::F(-0.26f, (float)Side * 0.34f, 1.08f), 0.042f, 0.12f, Hex(0x050505));
		}
		VisBox(C, C.Steer, FVisualBuildCtx::F(-0.16f, 0.f, 0.92f), FVector(0.02f, 0.22f, 0.22f), Hex(0xEDEDED));
		VisSphere(C, C.Steer, FVisualBuildCtx::F(-0.145f, 0.f, 0.92f), 0.08f, Hex(0xFFF3C4));
		VisBox(C, C.Steer, FVisualBuildCtx::F(0.f, 0.f, R * 2.f + 0.04f), FVector(0.5f, 0.12f, 0.02f), Primary);

		VisSeatedRider(C, FVector(0.52f, 0.f, 0.98f), FVector(0.78f, 0.16f, 0.74f), FVector(0.47f, 0.17f, 0.4f), FVector(0.78f, 0.f, 1.45f),
			WB - 0.27f, 0.33f, 1.08f, RC);

		Out.DeckPoint = C.P(0.68f, 0.f, 0.66f);
		Out.MotorPoint = C.P(0.5f, 0.f, 0.42f);
		Out.ControllerPoint = C.P(0.9f, 0.f, 0.75f);
	}

	void VisBuildBicycle(FVisualBuildCtx& C, const ScooterSim::FVehicleDef& Def, const FRiderColors& RC, FScooterVisual& Out)
	{
		using namespace ScooterShapes;
		const float WB = Def.WheelbaseM;
		const float R = Def.WheelRadiusM;
		const FLinearColor Primary = Hex(Def.ColorPrimary);
		const FLinearColor Secondary = Hex(Def.ColorSecondary);
		const FLinearColor Black = Hex(0x111111);
		const FLinearColor Metal = Hex(0xB0B0B0);
		const float T = 0.04f;

		Out.RearSpin = VisWheel(C, C.Body, C.P(0.f, 0.f, R), R, 0.045f, Metal, true);
		const FVector BB = C.P(0.45f, 0.f, 0.3f);
		const FVector SeatT = C.P(0.37f, 0.f, 0.88f);
		const FVector HeadTop = C.P(WB - 0.13f, 0.f, 0.95f);
		const FVector HeadBot = C.P(WB - 0.1f, 0.f, 0.78f);
		VisBeam(C, C.Body, BB, SeatT, T, Primary);
		VisBeam(C, C.Body, C.P(0.37f, 0.f, 0.86f), HeadTop, T, Primary);
		VisBeam(C, C.Body, BB, HeadBot, T * 1.3f, Primary);
		VisBeam(C, C.Body, HeadBot, HeadTop, T * 1.2f, Primary);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float Y = (float)Side * 0.05f;
			VisBeam(C, C.Body, C.P(0.f, Y, R), BB + FVector(0.f, Y * 60.f, 0.f), T * 0.8f, Primary);
			VisBeam(C, C.Body, C.P(0.f, Y, R), C.P(0.37f, Y * 0.5f, 0.84f), T * 0.8f, Primary);
		}
		VisBeam(C, C.Body, FMath::Lerp(BB, HeadBot, 0.15f) + FVector(0.f, 0.f, 6.f), FMath::Lerp(BB, HeadBot, 0.7f) + FVector(0.f, 0.f, 6.f), 0.08f, Secondary);
		VisBeam(C, C.Body, SeatT, C.P(0.35f, 0.f, 0.96f), 0.03f, Metal);
		VisBox(C, C.Body, C.P(0.34f, 0.f, 0.98f), FVector(0.26f, 0.14f, 0.05f), Black);
		VisBox(C, C.Body, C.P(0.08f, 0.f, R * 2.f + 0.1f), FVector(0.36f, 0.14f, 0.02f), Metal);
		VisCrossCylinder(C, C.Body, C.P(0.f, 0.f, R), 0.18f, 0.08f, Hex(0x2E2E2E));
		VisBox(C, C.Body, BB, FVector(0.18f, 0.24f, 0.03f), Black);

		C.Steer = ScooterShapes::AddPivot(C.Owner, C.Body, C.P(WB, 0.f, 0.f));
		VisTrack(C, C.Steer);
		Out.SteerPivot = C.Steer;
		Out.FrontSpin = VisWheel(C, C.Steer, FVisualBuildCtx::F(0.f, 0.f, R), R, 0.04f, Metal, true);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			VisBeam(C, C.Steer, FVisualBuildCtx::F(0.f, (float)Side * 0.045f, R), FVisualBuildCtx::F(-0.1f, (float)Side * 0.03f, 0.82f), 0.03f, Metal);
		}
		VisBeam(C, C.Steer, FVisualBuildCtx::F(-0.12f, 0.f, 0.95f), FVisualBuildCtx::F(-0.08f, 0.f, 1.06f), 0.035f, Black);
		VisCrossCylinder(C, C.Steer, FVisualBuildCtx::F(-0.06f, 0.f, 1.06f), 0.03f, 0.62f, Black);
		VisSphere(C, C.Steer, FVisualBuildCtx::F(-0.02f, 0.f, 0.86f), 0.06f, Hex(0xFFF3C4));

		VisSeatedRider(C, FVector(0.34f, 0.f, 1.03f), FVector(0.58f, 0.14f, 0.62f), FVector(0.45f, 0.12f, 0.25f), FVector(0.62f, 0.f, 1.5f),
			WB - 0.06f, 0.27f, 1.06f, RC);

		Out.DeckPoint = C.P(0.6f, 0.f, 0.6f);
		Out.MotorPoint = C.P(0.f, 0.f, R);
		Out.ControllerPoint = C.P(0.6f, 0.f, 0.55f);
	}
}

namespace ScooterVisual
{
	void Build(AActor* Owner, USceneComponent* Parent, int32 VehicleIndex, float RootHeightCm, const FLinearColor& Jacket,
		const FLinearColor& Helmet, FScooterVisual& Out, TArray<TObjectPtr<USceneComponent>>& OutCreated)
	{
		Clear(OutCreated);
		Out = FScooterVisual();
		if (!Owner || !Parent)
		{
			return;
		}
		const ScooterSim::FVehicleDef& Def = ScooterSim::Vehicle(VehicleIndex);
		Out.WheelbaseCm = Def.WheelbaseM * 100.f;
		Out.WheelRadiusCm = Def.WheelRadiusM * 100.f;
		Out.CgFromRearCm = Def.CgFromRearM * 100.f;
		Out.CgHeightCm = Def.CgHeightM * 100.f;

		const FVector PivotLoc(Out.CgFromRearCm - Out.WheelbaseCm * 0.5f, 0.f, Out.CgHeightCm - RootHeightCm);
		Out.PitchPivot = ScooterShapes::AddPivot(Owner, Parent, PivotLoc);
		Out.LeanPivot = ScooterShapes::AddPivot(Owner, Out.PitchPivot, FVector(0.f, 0.f, -Out.CgHeightCm));
		Out.RiderRoot = ScooterShapes::AddPivot(Owner, Out.LeanPivot, FVector::ZeroVector);
		OutCreated.Add(Out.PitchPivot);
		OutCreated.Add(Out.LeanPivot);
		OutCreated.Add(Out.RiderRoot);

		FVisualBuildCtx C;
		C.Owner = Owner;
		C.Body = Out.LeanPivot;
		C.Rider = Out.RiderRoot;
		C.Lr = Def.CgFromRearM;
		C.Created = &OutCreated;

		FRiderColors RC;
		RC.Jacket = Jacket;
		RC.Helmet = Helmet;

		switch (Def.Style)
		{
		case ScooterSim::EVisualStyle::Moto: VisBuildMoto(C, Def, RC, Out); break;
		case ScooterSim::EVisualStyle::Bicycle: VisBuildBicycle(C, Def, RC, Out); break;
		default: VisBuildScooter(C, Def, RC, Out); break;
		}
	}

	void Clear(TArray<TObjectPtr<USceneComponent>>& Created)
	{
		for (int32 i = Created.Num() - 1; i >= 0; --i)
		{
			USceneComponent* Comp = Created[i];
			if (IsValid(Comp))
			{
				Comp->DestroyComponent();
			}
		}
		Created.Reset();
	}

	void Pose(const FScooterVisual& V, const FVector& PivotLocation, float PitchDeg, float LeanDeg, float SteerDeg,
		float WheelAngleDeg, float RiderShiftCm)
	{
		if (!V.PitchPivot)
		{
			return;
		}
		V.PitchPivot->SetRelativeLocationAndRotation(PivotLocation, FRotator(PitchDeg, 0.f, 0.f));
		if (V.LeanPivot)
		{
			V.LeanPivot->SetRelativeRotation(FRotator(0.f, 0.f, LeanDeg));
		}
		if (V.SteerPivot)
		{
			V.SteerPivot->SetRelativeRotation(FRotator(0.f, SteerDeg, 0.f));
		}
		if (V.RearSpin)
		{
			V.RearSpin->SetRelativeRotation(FRotator(-WheelAngleDeg, 0.f, 0.f));
		}
		if (V.FrontSpin)
		{
			V.FrontSpin->SetRelativeRotation(FRotator(-WheelAngleDeg, 0.f, 0.f));
		}
		if (V.RiderRoot)
		{
			V.RiderRoot->SetRelativeLocation(FVector(RiderShiftCm, 0.f, 0.f));
		}
	}
}
