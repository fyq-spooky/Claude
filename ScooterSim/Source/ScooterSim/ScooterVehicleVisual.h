// Baut die sichtbare Fahrzeug-Huelle (Roller, E-Moto, Fahrrad + Fahrer) aus Grundformen.
#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;

struct FScooterVisual
{
	USceneComponent* PitchPivot = nullptr; // sitzt im Schwerpunkt, dreht fuer Wheelie/Hang
	USceneComponent* LeanPivot = nullptr;  // Boden unter dem Schwerpunkt, dreht fuer Schraeglage
	USceneComponent* RiderRoot = nullptr;  // Fahrer, verschiebt sich beim Zuruecklehnen
	USceneComponent* SteerPivot = nullptr; // Lenkung am Vorderrad
	USceneComponent* FrontSpin = nullptr;
	USceneComponent* RearSpin = nullptr;
	float WheelbaseCm = 100.f;
	float WheelRadiusCm = 12.f;
	float CgFromRearCm = 50.f;
	float CgHeightCm = 100.f;
	// Ungefaehre Positionen relativ zum LeanPivot fuer Effekte (Feuer, Rauch)
	FVector DeckPoint = FVector::ZeroVector;
	FVector MotorPoint = FVector::ZeroVector;
	FVector ControllerPoint = FVector::ZeroVector;
};

namespace ScooterVisual
{
	// RootHeightCm: Hoehe des Parent-Ursprungs ueber dem Boden (Pawn: Mitte der Kollisionsbox, Bot: 0).
	void Build(AActor* Owner, USceneComponent* Parent, int32 VehicleIndex, float RootHeightCm, const FLinearColor& Jacket,
		const FLinearColor& Helmet, FScooterVisual& Out, TArray<TObjectPtr<USceneComponent>>& OutCreated);

	void Clear(TArray<TObjectPtr<USceneComponent>>& Created);

	// Pitch/Lean/Steer in Grad, WheelAngle in Grad, RiderShift in cm (negativ = nach hinten).
	void Pose(const FScooterVisual& V, const FVector& PivotLocation, float PitchDeg, float LeanDeg, float SteerDeg,
		float WheelAngleDeg, float RiderShiftCm);
}
