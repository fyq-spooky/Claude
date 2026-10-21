// Hilfsfunktionen: alles im Spiel wird aus den Engine-Grundformen (Wuerfel, Kugel, Zylinder)
// zusammengebaut, damit das Projekt ohne eigene Assets laeuft.
#pragma once

#include "CoreMinimal.h"
#include "EngineUtils.h"

class AActor;
class UWorld;
class USceneComponent;
class UPrimitiveComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UTextRenderComponent;
class UPointLightComponent;

enum class EScooterShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone
};

namespace ScooterShapes
{
	UStaticMesh* Mesh(EScooterShape Shape);
	UMaterialInterface* BaseMaterial();
	FLinearColor Hex(uint32 Rgb);
	void ApplyColor(UPrimitiveComponent* Comp, const FLinearColor& Color);

	// Positionen und Groessen in cm, relativ zu Parent.
	UStaticMeshComponent* AddShape(AActor* Owner, USceneComponent* Parent, EScooterShape Shape, const FVector& Location,
		const FVector& Size, const FLinearColor& Color, const FRotator& Rotation = FRotator::ZeroRotator, bool bCollision = false);

	// Quader von A nach B (z.B. Arme, Rahmenrohre).
	UStaticMeshComponent* AddBeam(AActor* Owner, USceneComponent* Parent, const FVector& From, const FVector& To,
		float ThicknessCm, const FLinearColor& Color);

	UInstancedStaticMeshComponent* AddInstanced(AActor* Owner, USceneComponent* Parent, EScooterShape Shape,
		const FLinearColor& Color, bool bCollision, bool bCastShadow = true);

	USceneComponent* AddPivot(AActor* Owner, USceneComponent* Parent, const FVector& Location,
		const FRotator& Rotation = FRotator::ZeroRotator);

	UTextRenderComponent* AddText(AActor* Owner, USceneComponent* Parent, const FString& Text, const FVector& Location,
		const FRotator& Rotation, float WorldSize, const FColor& Color);

	UPointLightComponent* AddLight(AActor* Owner, USceneComponent* Parent, const FVector& Location, const FLinearColor& Color,
		float IntensityCandela, float RadiusCm);

	// Transform fuer eine Instanz mit Mittelpunkt und Groesse in cm.
	FTransform BoxTransform(const FVector& Center, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator);

	template <typename T>
	T* FindFirst(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<T> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}
}
