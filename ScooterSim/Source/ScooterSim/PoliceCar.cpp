#include "PoliceCar.h"
#include "PoliceManager.h"
#include "ScooterWorld.h"
#include "ScooterShapes.h"
#include "ScooterPawn.h"
#include "MotorSynthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

namespace
{
	constexpr float PoliceHalfHeight = 70.f;
	constexpr float PoliceRootZ = 15.f + PoliceHalfHeight;
}

APoliceCar::APoliceCar()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetBoxExtent(FVector(230.f, 98.f, PoliceHalfHeight));
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Collision->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Collision);

	Siren = CreateDefaultSubobject<UMotorSynthComponent>(TEXT("Siren"));
	Siren->SetupAttachment(Collision);
}

void APoliceCar::BeginPlay()
{
	Super::BeginPlay();
	BuildBody();
}

void APoliceCar::BuildBody()
{
	if (bBodyBuilt)
	{
		return;
	}
	bBodyBuilt = true;
	if (Siren)
	{
		Siren->Start();
	}
	using namespace ScooterShapes;
	USceneComponent* R = Collision;
	const float Base = -PoliceRootZ;
	const FLinearColor White = Hex(0xEDEFF2);
	const FLinearColor Blue = Hex(0x1F4FA8);
	const FLinearColor Glass = Hex(0x18222E);
	const FLinearColor Black = Hex(0x101010);

	AddShape(this, R, EScooterShape::Cube, FVector(0.f, 0.f, Base + 60.f), FVector(470.f, 192.f, 70.f), White);
	AddShape(this, R, EScooterShape::Cube, FVector(-20.f, 0.f, Base + 122.f), FVector(250.f, 176.f, 56.f), White);
	AddShape(this, R, EScooterShape::Cube, FVector(-20.f, 0.f, Base + 124.f), FVector(255.f, 180.f, 40.f), Glass);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		AddShape(this, R, EScooterShape::Cube, FVector(0.f, (float)Side * 97.f, Base + 60.f), FVector(472.f, 3.f, 18.f), Blue);
	}
	AddShape(this, R, EScooterShape::Cube, FVector(232.f, 0.f, Base + 55.f), FVector(8.f, 170.f, 30.f), Black);
	AddShape(this, R, EScooterShape::Cube, FVector(236.f, -65.f, Base + 70.f), FVector(4.f, 34.f, 14.f), Hex(0xFFF6D8));
	AddShape(this, R, EScooterShape::Cube, FVector(236.f, 65.f, Base + 70.f), FVector(4.f, 34.f, 14.f), Hex(0xFFF6D8));
	AddShape(this, R, EScooterShape::Cube, FVector(-236.f, 0.f, Base + 65.f), FVector(4.f, 160.f, 12.f), Hex(0xB01010));

	// Blaulicht-Balken
	AddShape(this, R, EScooterShape::Cube, FVector(-15.f, 0.f, Base + 156.f), FVector(40.f, 130.f, 12.f), Black);
	BeaconA = AddShape(this, R, EScooterShape::Cube, FVector(-15.f, -40.f, Base + 166.f), FVector(30.f, 42.f, 14.f), Hex(0x1040FF));
	BeaconB = AddShape(this, R, EScooterShape::Cube, FVector(-15.f, 40.f, Base + 166.f), FVector(30.f, 42.f, 14.f), Hex(0x1040FF));
	BeaconLightA = AddLight(this, R, FVector(-15.f, -40.f, Base + 185.f), FLinearColor(0.1f, 0.25f, 1.f), 0.f, 2500.f);
	BeaconLightB = AddLight(this, R, FVector(-15.f, 40.f, Base + 185.f), FLinearColor(0.1f, 0.25f, 1.f), 0.f, 2500.f);

	AddText(this, R, TEXT("POLIZEI"), FVector(0.f, -99.f, Base + 92.f), FRotator(0.f, -90.f, 0.f), 34.f, FColor(20, 70, 170));
	AddText(this, R, TEXT("POLIZEI"), FVector(0.f, 99.f, Base + 92.f), FRotator(0.f, 90.f, 0.f), 34.f, FColor(20, 70, 170));
	AddText(this, R, TEXT("POLIZEI"), FVector(150.f, 0.f, Base + 96.f), FRotator(90.f, 0.f, 0.f), 40.f, FColor(20, 70, 170));

	const float WX = 150.f;
	const float WY = 88.f;
	const float WR = 34.f;
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector P((i < 2) ? WX : -WX, (i % 2 == 0) ? -WY : WY, Base + WR);
		USceneComponent* Spin = AddPivot(this, R, P);
		AddShape(this, Spin, EScooterShape::Cylinder, FVector::ZeroVector, FVector(WR * 2.f, WR * 2.f, 24.f), Black, FRotator(0.f, 0.f, 90.f));
		AddShape(this, Spin, EScooterShape::Cube, FVector::ZeroVector, FVector(WR * 1.4f, 26.f, 6.f), Hex(0x8A8A8A));
		Wheels.Add(Spin);
	}
}

void APoliceCar::InitCar(AScooterWorld* InWorld, APoliceManager* InManager, int32 StartNode, int32 Seed)
{
	City = InWorld;
	Manager = InManager;
	Rng.Initialize(Seed);
	BuildBody();
	PlaceAtNode(StartNode);
}

void APoliceCar::PlaceAtNode(int32 Node)
{
	if (!City.IsValid())
	{
		return;
	}
	TargetNode = City->RandomNeighbor(Node, -1, Rng);
	PrevNode = Node;
	const FVector P = City->GetNode(Node);
	const FVector T = City->GetNode(TargetNode);
	Yaw = (float)(T - P).Rotation().Yaw;
	Speed = 0.f;
	SetActorLocationAndRotation(FVector(P.X, P.Y, PoliceRootZ + 3.f), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
}

void APoliceCar::SetChasing(bool bInChasing, float InTopSpeedMs)
{
	bChasing = bInChasing;
	TopSpeed = bChasing ? InTopSpeedMs : 11.f;
	if (!bChasing)
	{
		SetSirenVolume(0.f);
		if (BeaconLightA) BeaconLightA->SetIntensity(0.f);
		if (BeaconLightB) BeaconLightB->SetIntensity(0.f);
	}
}

void APoliceCar::SetSirenVolume(float Volume)
{
	if (Siren)
	{
		Siren->SetSiren(Volume);
	}
}

bool APoliceCar::HasLineOfSightTo(const FVector& Target) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FHitResult Hit;
	const FVector From = GetActorLocation() + FVector(0.f, 0.f, 80.f);
	const FVector To = Target + FVector(0.f, 0.f, 40.f);
	FCollisionQueryParams Params(FName(TEXT("PoliceSight")), false, this);
	return !World->LineTraceSingleByObjectType(Hit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
}

FVector APoliceCar::ChooseTarget(float& OutDesiredSpeed)
{
	const FVector Loc = GetActorLocation();
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (bChasing && Player && City.IsValid())
	{
		const FVector PP = Player->GetActorLocation();
		const float DistM = (float)FVector::Dist2D(PP, Loc) / 100.f;
		const bool bPlayerInPark = City->IsInPark(PP);
		if (DistM < 70.f && !bPlayerInPark && HasLineOfSightTo(PP))
		{
			// Direkte Verfolgung mit etwas Vorhalt
			const FVector Lead = Player->GetVelocity() * FMath::Clamp(DistM / 30.f, 0.f, 0.8f);
			OutDesiredSpeed = TopSpeed;
			if (DistM < 14.f)
			{
				const float PlayerSpeed = (float)Player->GetVelocity().Size() / 100.f;
				OutDesiredSpeed = FMath::Min(TopSpeed, PlayerSpeed + 4.f);
			}
			return PP + Lead;
		}
		const int32 Goal = City->FindNearestNode(PP);
		if (FVector::Dist2D(Loc, City->GetNode(TargetNode)) < 1200.0)
		{
			PrevNode = TargetNode;
			TargetNode = City->NextNodeToward(TargetNode, Goal);
			if (TargetNode == PrevNode)
			{
				// Spieler steht am Knoten (z.B. im Park): dort warten
				OutDesiredSpeed = bPlayerInPark ? 0.f : TopSpeed * 0.5f;
				return bPlayerInPark ? Loc + GetActorForwardVector() * 100.f : PP;
			}
		}
		OutDesiredSpeed = TopSpeed;
		return City->GetNode(TargetNode);
	}

	if (City.IsValid() && FVector::Dist2D(Loc, City->GetNode(TargetNode)) < 1000.0)
	{
		const int32 Next = City->RandomNeighbor(TargetNode, PrevNode, Rng);
		PrevNode = TargetNode;
		TargetNode = Next;
	}
	OutDesiredSpeed = 11.f;
	return City.IsValid() ? City->GetNode(TargetNode) : Loc;
}

void APoliceCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	RamCooldown = FMath::Max(0.f, RamCooldown - Dt);

	float Desired = 0.f;
	const FVector Target = ChooseTarget(Desired);
	const FVector Loc = GetActorLocation();
	const FVector ToTarget = Target - Loc;
	const float TargetYaw = (float)ToTarget.Rotation().Yaw;
	const float YawErr = FMath::FindDeltaAngleDegrees(Yaw, TargetYaw);

	// In Kurven abbremsen
	const float TurnFactor = FMath::Clamp(1.f - (FMath::Abs(YawErr) - 20.f) / 90.f, 0.25f, 1.f);
	Desired *= TurnFactor;

	if (ReverseTimer > 0.f)
	{
		ReverseTimer -= Dt;
		Speed = -4.f;
		Yaw += 50.f * Dt * (YawErr > 0.f ? -1.f : 1.f);
	}
	else
	{
		const float Accel = bChasing ? 6.f : 3.f;
		if (Speed < Desired)
		{
			Speed = FMath::Min(Desired, Speed + Accel * Dt);
		}
		else
		{
			Speed = FMath::Max(Desired, Speed - 9.f * Dt);
		}
		const float AbsV = FMath::Abs(Speed);
		const float MaxYawRate = AbsV < 0.5f ? 0.f : FMath::Min(70.f, FMath::RadiansToDegrees(FMath::Min(9.f / AbsV, AbsV * 0.26f)));
		Yaw += FMath::Clamp(YawErr * 3.f, -MaxYawRate, MaxYawRate) * Dt;
	}
	Yaw = (float)FRotator::NormalizeAxis(Yaw);

	const FRotator Rot(0.f, Yaw, 0.f);
	SetActorRotation(Rot);
	FHitResult Hit;
	SetActorLocation(Loc + Rot.Vector() * (Speed * 100.f * Dt), true, &Hit);
	if (Hit.bBlockingHit)
	{
		AScooterPawn* Scooter = Cast<AScooterPawn>(Hit.GetActor());
		if (Scooter && bChasing && RamCooldown <= 0.f)
		{
			RamCooldown = 2.f;
			if (Speed > 6.f)
			{
				Scooter->Crash(TEXT("Von der Polizei gerammt!"));
			}
		}
		Speed *= 0.3f;
	}

	if (bChasing && FMath::Abs(Speed) < 1.f && Desired > 3.f && ReverseTimer <= 0.f)
	{
		StuckTimer += Dt;
		if (StuckTimer > 1.5f)
		{
			StuckTimer = 0.f;
			ReverseTimer = 1.2f;
		}
	}
	else
	{
		StuckTimer = 0.f;
	}

	// Blaulicht + Raeder
	FlashTime += Dt;
	if (bChasing)
	{
		const bool bA = FMath::Fmod(FlashTime, 0.5f) < 0.25f;
		if (BeaconLightA) BeaconLightA->SetIntensity(bA ? 9000.f : 0.f);
		if (BeaconLightB) BeaconLightB->SetIntensity(bA ? 0.f : 9000.f);
	}
	WheelAngle = FMath::Fmod(WheelAngle + Speed * 100.f / 34.f * 57.3f * Dt, 360.f);
	for (USceneComponent* W : Wheels)
	{
		if (W)
		{
			W->SetRelativeRotation(FRotator(-WheelAngle, 0.f, 0.f));
		}
	}
}
