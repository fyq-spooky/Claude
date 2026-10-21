#include "RacerBot.h"
#include "Components/SceneComponent.h"

namespace SS = ScooterSim;

ARacerBot::ARacerBot()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	BotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BotRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(BotRoot);
	BotSetup = SS::MakeStockSetup(0);
	BotState = SS::FreshState();
}

void ARacerBot::Setup(int32 InVehicleIndex, const SS::FVehicleSetup& InSetup, const TArray<FVector>& InPath, float InSkill,
	const FLinearColor& Jacket, const FLinearColor& Helmet, const FVector& StartLocation, float StartYaw)
{
	VehicleIndex = InVehicleIndex;
	BotSetup = InSetup;
	BotState = SS::FreshState();
	Ride = SS::FRideState();
	Path = InPath;
	NextIndex = 0;
	Skill = InSkill;
	Yaw = StartYaw;
	bRunning = false;
	bFinished = false;
	SetActorLocationAndRotation(FVector(StartLocation.X, StartLocation.Y, 3.0), FRotator(0.f, Yaw, 0.f));
	ScooterVisual::Build(this, BotRoot, VehicleIndex, 0.f, Jacket, Helmet, Visual, VisualParts);
	ScooterVisual::Pose(Visual, FVector(Visual.CgFromRearCm - Visual.WheelbaseCm * 0.5f, 0.f, Visual.CgHeightCm), 0.f, 0.f, 0.f, 0.f, 0.f);
}

void ARacerBot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRunning || bFinished || Path.Num() == 0)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	const SS::FVehicleDef& Def = SS::Vehicle(VehicleIndex);
	const SS::FDerived D = SS::Derive(Def, BotSetup);

	const FVector Loc = GetActorLocation();
	const FVector Target = Path[NextIndex];
	const float DistM = (float)FVector::Dist2D(Loc, Target) / 100.f;

	// Kurvengeschwindigkeit am naechsten Wegpunkt
	float CornerSpeed = 1000.f;
	if (NextIndex + 1 < Path.Num())
	{
		const FVector Prev = NextIndex > 0 ? Path[NextIndex - 1] : Loc;
		const FVector A = (Target - Prev).GetSafeNormal2D();
		const FVector B = (Path[NextIndex + 1] - Target).GetSafeNormal2D();
		const float Turn = FMath::Acos(FMath::Clamp((float)FVector::DotProduct(A, B), -1.f, 1.f));
		if (Turn > 0.2f)
		{
			const float Radius = 16.f * (UE_HALF_PI / Turn);
			CornerSpeed = FMath::Sqrt(D.Grip * SS::Gravity * 0.85f * Radius);
		}
	}
	const float V = Ride.Speed;
	const float BrakeDecel = FMath::Min(D.BrakeDecel, D.Grip * SS::Gravity) * 0.8f;
	const float NeedDist = (V * V - CornerSpeed * CornerSpeed) / (2.f * FMath::Max(BrakeDecel, 1.f));
	float Throttle = Skill;
	float Brake = 0.f;
	if (V > CornerSpeed && DistM < NeedDist + 4.f)
	{
		Throttle = 0.f;
		Brake = 1.f;
	}

	const int32 Steps = 4;
	SS::FDriveInputs In;
	In.Throttle = Throttle;
	In.Brake = Brake;
	In.Lean = -0.3f;
	In.SlopeSin = 0.f;
	In.SlopeCos = 1.f;
	In.bGrounded = true;
	In.Dt = Dt / (float)Steps;
	for (int32 i = 0; i < Steps; ++i)
	{
		SS::FTelemetry Out;
		SS::StepVehicle(Def, D, BotState, Ride, In, Out);
		Ride.Pitch = 0.f;
		Ride.PitchRate = 0.f;
	}
	if (BotState.Failure != SS::EFailure::None)
	{
		// Gegner haben eine gute Werkstatt
		BotState = SS::FreshState();
	}
	BotState.AhUsed = 0.f;

	const float TargetYaw = (float)(Target - Loc).Rotation().Yaw;
	const float Err = FMath::FindDeltaAngleDegrees(Yaw, TargetYaw);
	const float MaxRate = FMath::RadiansToDegrees(FMath::Min(D.Grip * SS::Gravity / FMath::Max(FMath::Abs(Ride.Speed), 2.f), 2.5f));
	const float YawStep = FMath::Clamp(Err * 4.f, -MaxRate, MaxRate) * Dt;
	Yaw = (float)FRotator::NormalizeAxis(Yaw + YawStep);
	const FRotator Rot(0.f, Yaw, 0.f);
	SetActorLocationAndRotation(Loc + Rot.Vector() * (Ride.Speed * 100.f * Dt), Rot);

	if (DistM < 9.f)
	{
		++NextIndex;
		if (NextIndex >= Path.Num())
		{
			bFinished = true;
			NextIndex = Path.Num() - 1;
		}
	}

	const float YawRate = Dt > 0.f ? FMath::DegreesToRadians(YawStep / Dt) : 0.f;
	LeanDeg = FMath::FInterpTo(LeanDeg, FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Ride.Speed * YawRate, SS::Gravity)), -45.f, 45.f), Dt, 6.f);
	WheelAngle = FMath::Fmod(WheelAngle + FMath::RadiansToDegrees(Ride.Speed / Def.WheelRadiusM) * Dt, 360.f);
	ScooterVisual::Pose(Visual, FVector(Visual.CgFromRearCm - Visual.WheelbaseCm * 0.5f, 0.f, Visual.CgHeightCm), 0.f, LeanDeg,
		FMath::Clamp(Err, -20.f, 20.f), WheelAngle, 0.f);
}
