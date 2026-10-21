#include "ScooterPawn.h"
#include "ScooterPlayerController.h"
#include "ScooterWorld.h"
#include "ScooterShapes.h"
#include "MotorSynthComponent.h"
#include "FireFx.h"
#include "PoliceCar.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

namespace SS = ScooterSim;

namespace
{
	const FLinearColor PawnJacket(0.05f, 0.05f, 0.06f);
	const FLinearColor PawnHelmet(0.9f, 0.35f, 0.02f);
}

AScooterPawn::AScooterPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetBoxExtent(FVector(30.f, 20.f, BoxHalfHeightCm));
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Collision->SetSimulatePhysics(false);
	Collision->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Collision);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Collision);
	SpringArm->TargetArmLength = 430.f;
	SpringArm->SocketOffset = FVector(0.f, 0.f, 110.f);
	SpringArm->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 9.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 7.f;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritYaw = true;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->SetFieldOfView(88.f);

	MotorSound = CreateDefaultSubobject<UMotorSynthComponent>(TEXT("MotorSound"));
	MotorSound->SetupAttachment(Collision);
}

void AScooterPawn::BeginPlay()
{
	Super::BeginPlay();
	if (MotorSound)
	{
		MotorSound->Start();
	}
	if (AScooterWorld* City = ScooterShapes::FindFirst<AScooterWorld>(GetWorld()))
	{
		TeleportTo(City->GetSpawnLocation(), City->GetSpawnYaw());
	}
}

AScooterPlayerController* AScooterPawn::GetScooterController() const
{
	return Cast<AScooterPlayerController>(GetController());
}

void AScooterPawn::SetProfile(SS::FProfile* InProfile)
{
	Profile = InProfile;
	RebuildVehicle();
}

void AScooterPawn::RebuildVehicle()
{
	if (!Profile)
	{
		return;
	}
	EndWheelie(false);
	VehicleIndex = FMath::Clamp(Profile->Current, 0, SS::NumVehicles() - 1);
	ClearDamageFx();
	ScooterVisual::Build(this, Collision, VehicleIndex, RootHeightCm, PawnJacket, PawnHelmet, Visual, VisualParts);
	const SS::FVehicleDef& Def = SS::Vehicle(VehicleIndex);
	SpringArm->TargetArmLength = Def.Class == SS::EVehicleClass::Scooter ? 430.f : 480.f;
	Ride = SS::FRideState();
	SteerAngleRad = 0.f;
	LeanVisualDeg = 0.f;
	if (Profile->States[VehicleIndex].Failure != SS::EFailure::None)
	{
		SpawnFailureFx(Profile->States[VehicleIndex].Failure);
	}
	UpdateVisuals(0.f);
}

FVector AScooterPawn::GetForwardDir() const
{
	return FRotator(0.f, Yaw, 0.f).Vector();
}

FVector AScooterPawn::GetVelocity() const
{
	if (bAirborne)
	{
		return AirVelocity * 100.f;
	}
	return GetForwardDir() * (Ride.Speed * 100.f);
}

bool AScooterPawn::IsBroken() const
{
	return Profile && Profile->States[VehicleIndex].Failure != SS::EFailure::None;
}

float AScooterPawn::GetBalanceDeg() const
{
	return FMath::RadiansToDegrees(SS::BalancePointRad(SS::Vehicle(VehicleIndex), LeanSmoothed));
}

bool AScooterPawn::ProbeGround(const FVector& Point, float RefZ, float& OutZ) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FHitResult Hit;
	const FVector Start((float)Point.X, (float)Point.Y, RefZ + 120.f);
	const FVector End((float)Point.X, (float)Point.Y, RefZ - 400.f);
	FCollisionQueryParams Params(FName(TEXT("ScooterGround")), false, this);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	if (World->LineTraceSingleByObjectType(Hit, Start, End, Objects, Params))
	{
		OutZ = (float)Hit.ImpactPoint.Z;
		return true;
	}
	return false;
}

void AScooterPawn::TeleportTo(const FVector& GroundLocation, float YawDeg)
{
	Yaw = YawDeg;
	Ride = SS::FRideState();
	bAirborne = false;
	bCrashed = false;
	AirVelocity = FVector::ZeroVector;
	SteerAngleRad = 0.f;
	EndWheelie(false);
	float Z = (float)GroundLocation.Z;
	float Probe = 0.f;
	if (ProbeGround(GroundLocation, Z + 250.f, Probe))
	{
		Z = Probe;
		bHasGround = true;
	}
	FrontGroundZ = Z;
	RearGroundZ = Z;
	SlopeRad = 0.f;
	SetActorLocationAndRotation(FVector(GroundLocation.X, GroundLocation.Y, Z + RootHeightCm), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	LastSafeLocation = FVector(GroundLocation.X, GroundLocation.Y, Z);
	LastSafeYaw = Yaw;
	UpdateVisuals(0.f);
}

void AScooterPawn::ResetToLastSafe()
{
	TeleportTo(LastSafeLocation, LastSafeYaw);
}

void AScooterPawn::Crash(const FString& Reason)
{
	if (bCrashed)
	{
		return;
	}
	EndWheelie(false);
	if (bAirborne)
	{
		// In der Luft verunglueckt: auf den Boden fallen lassen
		float Z = 0.f;
		const FVector Loc = GetActorLocation();
		if (ProbeGround(Loc, (float)Loc.Z - RootHeightCm, Z))
		{
			FrontGroundZ = Z;
			RearGroundZ = Z;
			SlopeRad = 0.f;
			SetActorLocation(FVector(Loc.X, Loc.Y, Z + RootHeightCm));
		}
	}
	bCrashed = true;
	CrashTimer = 2.4f;
	bAirborne = false;
	Ride.Speed = 0.f;
	Ride.Pitch = 0.f;
	Ride.PitchRate = 0.f;
	if (AScooterPlayerController* PC = GetScooterController())
	{
		PC->OnCrash(Reason);
	}
}

void AScooterPawn::ClearDamageFx()
{
	for (AFireFx* Fx : DamageFx)
	{
		if (IsValid(Fx))
		{
			Fx->Destroy();
		}
	}
	DamageFx.Reset();
	if (IsValid(SmokeFx))
	{
		SmokeFx->Destroy();
	}
	SmokeFx = nullptr;
}

void AScooterPawn::SpawnFailureFx(SS::EFailure Failure)
{
	UWorld* World = GetWorld();
	if (!World || !Visual.LeanPivot || Failure == SS::EFailure::None)
	{
		return;
	}
	FVector Point = Visual.DeckPoint;
	float Scale = 1.f;
	switch (Failure)
	{
	case SS::EFailure::PhaseWire:
	case SS::EFailure::Motor:
		Point = Visual.MotorPoint;
		break;
	case SS::EFailure::Controller:
	case SS::EFailure::ControllerOvervoltage:
		Point = Visual.ControllerPoint;
		Scale = 1.4f;
		break;
	case SS::EFailure::BatteryFire:
		Scale = 3.f;
		break;
	default:
		break;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFireFx* Fx = World->SpawnActor<AFireFx>(AFireFx::StaticClass(), FTransform::Identity, Params);
	if (Fx)
	{
		Fx->Setup(Scale, false, 0.f);
		Fx->AttachToComponent(Visual.LeanPivot, FAttachmentTransformRules::KeepRelativeTransform);
		Fx->SetActorRelativeLocation(Point);
		DamageFx.Add(Fx);
	}
}

// ---------------------------------------------------------------------------------------------

void AScooterPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Profile)
	{
		return;
	}
	const float Dt = FMath::Clamp(DeltaSeconds, 0.0005f, 0.05f);
	const SS::FVehicleDef& Def = SS::Vehicle(VehicleIndex);
	const SS::FDerived D = SS::Derive(Def, Profile->Setups[VehicleIndex]);
	BmsNoticeCooldown = FMath::Max(0.f, BmsNoticeCooldown - Dt);

	if (!bHasGround)
	{
		// Welt noch nicht da? Warten, bis der Boden gefunden wird.
		float Z = 0.f;
		if (ProbeGround(GetActorLocation(), (float)GetActorLocation().Z + 500.f, Z))
		{
			TeleportTo(FVector(GetActorLocation().X, GetActorLocation().Y, Z), Yaw);
		}
		return;
	}

	if (bCrashed)
	{
		CrashTimer -= Dt;
		CoolDown(Dt);
		if (CrashTimer <= 0.f)
		{
			ResetToLastSafe();
		}
		UpdateVisuals(Dt);
		UpdateSound(D);
		return;
	}
	if (IsFrozen())
	{
		Ride.Speed = 0.f;
		Ride.Pitch = 0.f;
		Ride.PitchRate = 0.f;
		Telemetry = SS::FTelemetry();
		CoolDown(Dt);
		UpdateVisuals(Dt);
		UpdateSound(D);
		return;
	}

	if (bAirborne)
	{
		TickAirborne(Dt, Def, D);
	}
	else
	{
		TickGrounded(Dt, Def, D);
	}
	HandleEvents(Dt);
	UpdateVisuals(Dt);
	UpdateSound(D);
}

void AScooterPawn::StepCore(float Dt, bool bGrounded, const SS::FVehicleDef& Def, const SS::FDerived& D, float Throttle, float Brake, float Lean)
{
	SS::FVehicleState& St = Profile->States[VehicleIndex];
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Dt * 240.f), 1, 12);
	SS::FDriveInputs In;
	In.Throttle = Throttle;
	In.Brake = Brake;
	In.Lean = Lean;
	In.SlopeSin = FMath::Sin(SlopeRad);
	In.SlopeCos = FMath::Cos(SlopeRad);
	In.bGrounded = bGrounded;
	In.Dt = Dt / (float)Steps;
	In.AirPathAngle = FMath::Atan2((float)AirVelocity.Z, (float)FVector(AirVelocity.X, AirVelocity.Y, 0.0).Size());

	SS::FTelemetry Merged = SS::FTelemetry();
	Merged.NewFailure = SS::EFailure::None;
	for (int32 i = 0; i < Steps; ++i)
	{
		SS::FTelemetry Out;
		SS::StepVehicle(Def, D, St, Ride, In, Out);
		const SS::EFailure Failure = Merged.NewFailure != SS::EFailure::None ? Merged.NewFailure : Out.NewFailure;
		const bool bLoop = Merged.bLoopOut || Out.bLoopOut;
		const bool bBms = Merged.bBmsCut || Out.bBmsCut;
		Merged = Out;
		Merged.NewFailure = Failure;
		Merged.bLoopOut = bLoop;
		Merged.bBmsCut = bBms;
		if (bLoop)
		{
			break;
		}
	}
	Telemetry = Merged;
}

void AScooterPawn::CoolDown(float Dt)
{
	SS::FVehicleState& St = Profile->States[VehicleIndex];
	for (int32 i = 0; i < SS::NumThermal; ++i)
	{
		const SS::FThermalSpec& Spec = SS::ThermalSpec((SS::EThermal)i);
		const float K = 1.f - FMath::Exp(-Dt / Spec.TimeConstS);
		St.Temp[i] += (SS::AmbientTempC - St.Temp[i]) * K;
	}
	St.LastBattV = 0.f;
}

void AScooterPawn::TickGrounded(float Dt, const SS::FVehicleDef& Def, const SS::FDerived& D)
{
	LeanSmoothed = FMath::FInterpTo(LeanSmoothed, Input.Lean, Dt, 7.f);

	// Rueckwaerts schieben: Bremse halten im Stand
	if (Input.Brake > 0.5f && Input.Throttle < 0.05f && Ride.Speed <= 0.1f)
	{
		ReverseHold += Dt;
	}
	else
	{
		ReverseHold = 0.f;
	}
	const bool bWalkBack = ReverseHold > 0.6f;
	StepCore(Dt, true, Def, D, bWalkBack ? 0.f : Input.Throttle, bWalkBack ? 0.f : Input.Brake, LeanSmoothed);
	if (bWalkBack)
	{
		Ride.Speed = -1.6f;
	}
	if (Telemetry.bLoopOut)
	{
		Crash(TEXT("\u00dcberschlag! Zu weit zur\u00fcckgelehnt."));
		return;
	}

	// ---- Lenkung
	const float V = Ride.Speed;
	const float AbsV = FMath::Abs(V);
	const float WB = Def.WheelbaseM;
	const float MaxSteer = FMath::DegreesToRadians(FMath::Lerp(34.f, 3.5f, FMath::Clamp(AbsV / 30.f, 0.f, 1.f)));
	SteerAngleRad = FMath::FInterpTo(SteerAngleRad, Input.Steer * MaxSteer, Dt, 10.f);
	float YawRate = V * FMath::Tan(SteerAngleRad) / WB;
	if (Ride.Pitch > 0.05f)
	{
		YawRate *= 0.35f;
	}
	const float MaxLat = D.Grip * SS::Gravity * 0.95f;
	if (AbsV > 0.5f && FMath::Abs(YawRate * AbsV) > MaxLat)
	{
		YawRate = FMath::Sign(YawRate) * MaxLat / AbsV;
	}
	Yaw = (float)FRotator::NormalizeAxis(Yaw + FMath::RadiansToDegrees(YawRate) * Dt);
	const float LeanTarget = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(V * YawRate, SS::Gravity)), -50.f, 50.f);
	LeanVisualDeg = FMath::FInterpTo(LeanVisualDeg, LeanTarget, Dt, 6.f);

	// ---- Bewegen
	const FRotator YawRot(0.f, Yaw, 0.f);
	SetActorRotation(YawRot);
	const FVector Fwd = YawRot.Vector();
	const FVector Start = GetActorLocation();
	const float HorizontalCm = Ride.Speed * FMath::Cos(SlopeRad) * 100.f * Dt;
	FHitResult Hit;
	SetActorLocation(Start + Fwd * HorizontalCm, true, &Hit);
	if (Hit.bBlockingHit && Hit.ImpactNormal.Z > 0.45f)
	{
		// Schraege Flaeche (Rampe) statt Wand: einfach weiterfahren, die Hoehe passt der Bodenabgleich an
		SetActorLocation(Start + Fwd * HorizontalCm, false);
	}
	else if (Hit.bBlockingHit && HandleImpact(Hit, Fwd))
	{
		return;
	}

	// ---- Boden folgen
	const FVector Loc = GetActorLocation();
	const float HalfWB = WB * 50.f;
	const float Moved = (float)FVector::Dist2D(Start, Loc);
	const float Rise = Moved * FMath::Tan(SlopeRad) * (Ride.Speed >= 0.f ? 1.f : -1.f);
	const float ExpF = FrontGroundZ + Rise;
	const float ExpR = RearGroundZ + Rise;
	float Zf = ExpF;
	float Zr = ExpR;
	const bool bF = ProbeGround(Loc + Fwd * HalfWB, ExpF, Zf) && Zf < ExpF + 35.f;
	const bool bR = ProbeGround(Loc - Fwd * HalfWB, ExpR, Zr) && Zr < ExpR + 35.f;
	const bool bDropF = !bF || (ExpF - Zf > 25.f);
	const bool bDropR = !bR || (ExpR - Zr > 25.f);
	if (bDropF && bDropR)
	{
		FrontGroundZ = ExpF;
		RearGroundZ = ExpR;
		SetActorLocation(FVector(Loc.X, Loc.Y, (ExpF + ExpR) * 0.5f + RootHeightCm));
		TakeOff(Fwd);
		return;
	}
	if (bDropF)
	{
		Zf = ExpF; // Vorderrad ueber der Kante: erst mal geradeaus weiter
	}
	if (bDropR)
	{
		Zr = ExpR;
	}
	FrontGroundZ = Zf;
	RearGroundZ = Zr;
	SlopeRad = FMath::Atan2(Zf - Zr, WB * 100.f);
	SetActorLocation(FVector(Loc.X, Loc.Y, (Zf + Zr) * 0.5f + RootHeightCm));

	// ---- Sicherer Punkt fuer Reset
	SafeTimer += Dt;
	if (SafeTimer > 0.5f && AbsV > 1.f && Ride.Pitch < 0.03f && FMath::Abs(SlopeRad) < 0.05f)
	{
		SafeTimer = 0.f;
		LastSafeLocation = FVector(Loc.X, Loc.Y, (Zf + Zr) * 0.5f);
		LastSafeYaw = Yaw;
	}
}

void AScooterPawn::TakeOff(const FVector& Fwd)
{
	bAirborne = true;
	EndWheelie(true);
	AirVelocity = Fwd * (Ride.Speed * FMath::Cos(SlopeRad)) + FVector(0.f, 0.f, Ride.Speed * FMath::Sin(SlopeRad));
	Ride.AirPitch = SlopeRad + Ride.Pitch;
	Ride.AirPitchRate = Ride.PitchRate;
	Ride.Pitch = 0.f;
	Ride.PitchRate = 0.f;
	AirTime = 0.f;
	AirRotation = 0.f;
}

void AScooterPawn::TickAirborne(float Dt, const SS::FVehicleDef& Def, const SS::FDerived& D)
{
	LeanSmoothed = FMath::FInterpTo(LeanSmoothed, Input.Lean, Dt, 7.f);
	StepCore(Dt, false, Def, D, Input.Throttle, Input.Brake, LeanSmoothed);
	AirTime += Dt;
	AirRotation += Ride.AirPitchRate * Dt;

	AirVelocity.Z -= SS::Gravity * Dt;
	const float SpeedAir = (float)AirVelocity.Size();
	const float DragK = 0.5f * SS::AirDensity * Def.CdA * SpeedAir / D.TotalMassKg;
	AirVelocity -= AirVelocity * FMath::Min(DragK * Dt, 0.5f);
	Ride.Speed = (float)FVector(AirVelocity.X, AirVelocity.Y, 0.0).Size();

	Yaw = (float)FRotator::NormalizeAxis(Yaw + Input.Steer * 25.f * Dt);
	SetActorRotation(FRotator(0.f, Yaw, 0.f));
	LeanVisualDeg = FMath::FInterpTo(LeanVisualDeg, Input.Steer * 15.f, Dt, 3.f);

	const FVector Start = GetActorLocation();
	FHitResult Hit;
	SetActorLocation(Start + AirVelocity * (100.f * Dt), true, &Hit);
	if (Hit.bBlockingHit)
	{
		const FVector N = Hit.ImpactNormal;
		const float Into = -(float)FVector::DotProduct(AirVelocity, N);
		if (N.Z < 0.6f)
		{
			if (Into > 7.f)
			{
				Crash(TEXT("In der Luft gegen ein Hindernis geknallt!"));
				return;
			}
			AirVelocity += N * Into;
		}
	}

	const FVector Loc = GetActorLocation();
	const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
	const float HalfWB = Def.WheelbaseM * 50.f;
	const float BottomZ = (float)Loc.Z - RootHeightCm;
	const float FrontWheelZ = BottomZ + FMath::Sin(Ride.AirPitch) * HalfWB;
	const float RearWheelZ = BottomZ - FMath::Sin(Ride.AirPitch) * HalfWB;
	float Zf = 0.f;
	float Zr = 0.f;
	const bool bF = ProbeGround(Loc + Fwd * HalfWB, FrontWheelZ, Zf);
	const bool bR = ProbeGround(Loc - Fwd * HalfWB, RearWheelZ, Zr);
	if ((bF && FrontWheelZ <= Zf + 3.f) || (bR && RearWheelZ <= Zr + 3.f) || (Hit.bBlockingHit && Hit.ImpactNormal.Z >= 0.6f))
	{
		Land(Zf, Zr, bF, bR, Fwd, Def);
	}
	else if (Loc.Z < -5000.f)
	{
		Crash(TEXT("Aus der Welt gefallen."));
	}
}

void AScooterPawn::Land(float Zf, float Zr, bool bHasFront, bool bHasRear, const FVector& Fwd, const SS::FVehicleDef& Def)
{
	if (!bHasFront && !bHasRear)
	{
		const float Z = (float)GetActorLocation().Z - RootHeightCm;
		Zf = Z;
		Zr = Z;
	}
	else if (!bHasFront)
	{
		Zf = Zr;
	}
	else if (!bHasRear)
	{
		Zr = Zf;
	}
	const float Slope = FMath::Atan2(Zf - Zr, Def.WheelbaseM * 100.f);
	const FVector Up(0.f, 0.f, 1.f);
	const FVector Along = Fwd * FMath::Cos(Slope) + Up * FMath::Sin(Slope);
	const FVector Normal = Fwd * -FMath::Sin(Slope) + Up * FMath::Cos(Slope);
	const float VAlong = (float)FVector::DotProduct(AirVelocity, Along);
	const float VInto = -(float)FVector::DotProduct(AirVelocity, Normal);
	const int32 Flips = FMath::RoundToInt(AirRotation / (2.f * SS::Pi));
	const float Rel = FMath::DegreesToRadians((float)FRotator::NormalizeAxis(FMath::RadiansToDegrees(Ride.AirPitch - Slope)));
	const float Balance = SS::BalancePointRad(Def, LeanSmoothed);
	const float Air = AirTime;

	bAirborne = false;
	FrontGroundZ = Zf;
	RearGroundZ = Zr;
	SlopeRad = Slope;
	const FVector Loc = GetActorLocation();
	SetActorLocation(FVector(Loc.X, Loc.Y, (Zf + Zr) * 0.5f + RootHeightCm));
	AirVelocity = FVector::ZeroVector;
	AirTime = 0.f;

	if (VInto > 11.f)
	{
		Crash(TEXT("Viel zu hart gelandet!"));
		return;
	}
	if (Rel < -0.6f)
	{
		Crash(TEXT("Mit dem Vorderrad zuerst eingeschlagen!"));
		return;
	}
	if (Rel > Balance + 0.25f)
	{
		Crash(TEXT("Auf dem R\u00fccken gelandet!"));
		return;
	}
	Ride.Speed = VAlong;
	if (Rel > 0.06f)
	{
		// Auf dem Hinterrad gelandet: weiter im Wheelie
		Ride.Pitch = Rel;
		Ride.PitchRate = 0.f;
	}
	else
	{
		Ride.Pitch = 0.f;
		Ride.PitchRate = 0.f;
	}
	if (AScooterPlayerController* PC = GetScooterController())
	{
		PC->OnStuntLanded(Air, Flips, FMath::Abs(VAlong) * 3.6f);
	}
}

bool AScooterPawn::HandleImpact(const FHitResult& Hit, const FVector& Fwd)
{
	FVector N = Hit.ImpactNormal;
	N.Z = 0.0;
	if (!N.Normalize())
	{
		return false;
	}
	const float Dot = (float)FVector::DotProduct(Fwd, N);
	const float IntoSpeed = -Dot * Ride.Speed;
	const AActor* Other = Hit.GetActor();
	const bool bPolice = Other && Other->IsA(APoliceCar::StaticClass());
	if (IntoSpeed > (bPolice ? 4.f : 6.5f))
	{
		Crash(bPolice ? TEXT("In die Polizei gekracht!") : TEXT("Voll gegen die Wand!"));
		return true;
	}
	if (FMath::Abs(Dot) < 0.75f)
	{
		// Streifschuss: an der Wand entlang rutschen
		const FVector Tangent = (Fwd - N * Dot).GetSafeNormal();
		if (!Tangent.IsNearlyZero())
		{
			Yaw = (float)Tangent.Rotation().Yaw;
		}
		Ride.Speed *= 1.f - FMath::Abs(Dot) * 0.8f;
	}
	else
	{
		Ride.Speed = 0.f;
	}
	AddActorWorldOffset(N * 2.0);
	return false;
}

void AScooterPawn::HandleEvents(float Dt)
{
	AScooterPlayerController* PC = GetScooterController();
	if (Telemetry.NewFailure != SS::EFailure::None)
	{
		EndWheelie(false);
		SpawnFailureFx(Telemetry.NewFailure);
		if (PC)
		{
			PC->OnVehicleFailure(Telemetry.NewFailure);
		}
		Telemetry.NewFailure = SS::EFailure::None;
	}
	if (Telemetry.bBmsCut && BmsNoticeCooldown <= 0.f && PC)
	{
		BmsNoticeCooldown = 3.f;
		PC->Notify(TEXT("BMS-ABSCHALTUNG! Akkustrom zu hoch f\u00fcr das BMS."), FLinearColor(1.f, 0.6f, 0.1f), 2.5f, false);
	}

	const bool bWheelieNow = !bAirborne && !bCrashed && Ride.Pitch > 0.07f && Ride.Speed > 1.5f;
	if (bWheelieNow)
	{
		bInWheelie = true;
		WheelieDist += Ride.Speed * Dt;
		WheelieTime += Dt;
		WheelieSpeedSum += Ride.Speed * Dt;
	}
	else if (bInWheelie)
	{
		EndWheelie(true);
	}

	SmokeTimer -= Dt;
	if (SmokeTimer <= 0.f)
	{
		SmokeTimer = 0.25f;
		UpdateSmoke();
	}
}

void AScooterPawn::EndWheelie(bool bPay)
{
	if (bInWheelie && bPay && WheelieDist > 3.f && WheelieTime > 0.f)
	{
		if (AScooterPlayerController* PC = GetScooterController())
		{
			PC->OnWheelieEnded(WheelieDist, WheelieTime, WheelieSpeedSum / WheelieTime);
		}
	}
	bInWheelie = false;
	WheelieDist = 0.f;
	WheelieTime = 0.f;
	WheelieSpeedSum = 0.f;
}

void AScooterPawn::UpdateSmoke()
{
	if (!Profile || !Visual.LeanPivot)
	{
		return;
	}
	const SS::FVehicleState& St = Profile->States[VehicleIndex];
	bool bHot = false;
	const SS::EThermal Parts[] = { SS::EThermal::PhaseWire, SS::EThermal::BatteryWire, SS::EThermal::Connector, SS::EThermal::Controller, SS::EThermal::Motor };
	for (SS::EThermal Part : Parts)
	{
		const SS::FThermalSpec& Spec = SS::ThermalSpec(Part);
		if (St.Temp[(int32)Part] > (Spec.WarnC + Spec.FailC) * 0.5f)
		{
			bHot = true;
		}
	}
	if (bHot && !IsValid(SmokeFx) && St.Failure == SS::EFailure::None)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SmokeFx = GetWorld()->SpawnActor<AFireFx>(AFireFx::StaticClass(), FTransform::Identity, Params);
		if (SmokeFx)
		{
			SmokeFx->Setup(1.f, true, 0.f);
			SmokeFx->AttachToComponent(Visual.LeanPivot, FAttachmentTransformRules::KeepRelativeTransform);
			SmokeFx->SetActorRelativeLocation(Visual.DeckPoint);
		}
	}
	else if ((!bHot || St.Failure != SS::EFailure::None) && IsValid(SmokeFx))
	{
		SmokeFx->Destroy();
		SmokeFx = nullptr;
	}
}

void AScooterPawn::UpdateVisuals(float Dt)
{
	if (!Visual.PitchPivot)
	{
		return;
	}
	const SS::FVehicleDef& Def = SS::Vehicle(VehicleIndex);
	const float Lr = Def.CgFromRearM * 100.f;
	const float H = Def.CgHeightM * 100.f;
	const float HalfWB = Def.WheelbaseM * 50.f;
	FVector Pivot;
	float PitchDeg = 0.f;
	if (bAirborne)
	{
		Pivot = FVector(Lr - HalfWB, 0.f, H - RootHeightCm);
		PitchDeg = FMath::RadiansToDegrees(Ride.AirPitch);
	}
	else
	{
		const float P = SlopeRad + Ride.Pitch;
		const float RearRel = RearGroundZ - (float)GetActorLocation().Z;
		Pivot = FVector(-HalfWB + Lr * FMath::Cos(P) - H * FMath::Sin(P), 0.f, RearRel + Lr * FMath::Sin(P) + H * FMath::Cos(P));
		PitchDeg = FMath::RadiansToDegrees(P);
	}
	float Lean = LeanVisualDeg;
	if (bCrashed)
	{
		Lean = FMath::Min(82.f, (2.4f - CrashTimer) * 260.f);
	}
	const float SpinBoost = Telemetry.bWheelspin ? 2.5f : 1.f;
	WheelAngleDeg = FMath::Fmod(WheelAngleDeg + FMath::RadiansToDegrees(Ride.Speed * SpinBoost / Def.WheelRadiusM) * Dt, 360.f);
	const float RiderShift = -LeanSmoothed * Def.LeanShiftM * 100.f;
	ScooterVisual::Pose(Visual, Pivot, PitchDeg, Lean, FMath::RadiansToDegrees(SteerAngleRad), WheelAngleDeg, RiderShift);

	if (Camera)
	{
		Camera->SetFieldOfView(88.f + FMath::Min(22.f, GetSpeedKmh() * 0.12f));
	}
}

void AScooterPawn::UpdateSound(const SS::FDerived& D)
{
	if (!MotorSound)
	{
		return;
	}
	if (IsFrozen() || bCrashed)
	{
		MotorSound->SetMotor(0.f, 0.f, 0.f);
		return;
	}
	const float AbsV = FMath::Abs(Ride.Speed);
	const float PolePairs = D.Motor->Kind == SS::EMotorKind::Hub ? 14.f : 5.f;
	const float Freq = 35.f + Telemetry.MotorRpm / 60.f * PolePairs * 0.5f;
	const float Load = FMath::Clamp(Telemetry.PhaseA / FMath::Max(D.PhaseA, 1.f), 0.f, 1.f);
	const bool bActive = AbsV > 0.3f || Telemetry.PhaseA > 1.f;
	const float Amp = bActive ? 0.03f + 0.22f * Load + 0.03f * FMath::Clamp(AbsV / 20.f, 0.f, 1.f) : 0.f;
	const float Wind = FMath::Clamp(FMath::Square(AbsV / 45.f), 0.f, 0.45f) + (Telemetry.bWheelspin ? 0.15f : 0.f);
	MotorSound->SetMotor(Freq, Amp, Wind);
}
