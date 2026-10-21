#include "ScooterShapes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace ScooterShapes
{
	UStaticMesh* Mesh(EScooterShape Shape)
	{
		const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
		switch (Shape)
		{
		case EScooterShape::Sphere: Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
		case EScooterShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
		case EScooterShape::Cone: Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
		default: break;
		}
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	UMaterialInterface* BaseMaterial()
	{
		return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	FLinearColor Hex(uint32 Rgb)
	{
		return FLinearColor(FColor((uint8)((Rgb >> 16) & 0xFF), (uint8)((Rgb >> 8) & 0xFF), (uint8)(Rgb & 0xFF), 255));
	}

	void ApplyColor(UPrimitiveComponent* Comp, const FLinearColor& Color)
	{
		UMaterialInterface* Base = BaseMaterial();
		if (!Comp || !Base)
		{
			return;
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Comp);
		if (Mid)
		{
			Mid->SetVectorParameterValue(TEXT("Color"), Color);
			Comp->SetMaterial(0, Mid);
		}
	}

	UStaticMeshComponent* AddShape(AActor* Owner, USceneComponent* Parent, EScooterShape Shape, const FVector& Location,
		const FVector& Size, const FLinearColor& Color, const FRotator& Rotation, bool bCollision)
	{
		if (!Owner)
		{
			return nullptr;
		}
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(Mesh(Shape));
		Comp->SetRelativeLocation(Location);
		Comp->SetRelativeRotation(Rotation);
		Comp->SetRelativeScale3D(Size / 100.0);
		if (bCollision)
		{
			Comp->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Comp->SetGenerateOverlapEvents(false);
		if (Parent)
		{
			Comp->SetupAttachment(Parent);
		}
		Comp->RegisterComponent();
		ApplyColor(Comp, Color);
		Owner->AddInstanceComponent(Comp);
		return Comp;
	}

	UStaticMeshComponent* AddBeam(AActor* Owner, USceneComponent* Parent, const FVector& From, const FVector& To,
		float ThicknessCm, const FLinearColor& Color)
	{
		const FVector Delta = To - From;
		const double Length = FMath::Max(Delta.Size(), 1.0);
		const FRotator Rot = Delta.Rotation();
		return AddShape(Owner, Parent, EScooterShape::Cube, (From + To) * 0.5, FVector(Length, ThicknessCm, ThicknessCm), Color, Rot);
	}

	UInstancedStaticMeshComponent* AddInstanced(AActor* Owner, USceneComponent* Parent, EScooterShape Shape,
		const FLinearColor& Color, bool bCollision, bool bCastShadow)
	{
		if (!Owner)
		{
			return nullptr;
		}
		UInstancedStaticMeshComponent* Comp = NewObject<UInstancedStaticMeshComponent>(Owner);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(Mesh(Shape));
		if (bCollision)
		{
			Comp->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCastShadow(bCastShadow);
		if (Parent)
		{
			Comp->SetupAttachment(Parent);
		}
		Comp->RegisterComponent();
		ApplyColor(Comp, Color);
		Owner->AddInstanceComponent(Comp);
		return Comp;
	}

	USceneComponent* AddPivot(AActor* Owner, USceneComponent* Parent, const FVector& Location, const FRotator& Rotation)
	{
		if (!Owner)
		{
			return nullptr;
		}
		USceneComponent* Comp = NewObject<USceneComponent>(Owner);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetRelativeLocation(Location);
		Comp->SetRelativeRotation(Rotation);
		if (Parent)
		{
			Comp->SetupAttachment(Parent);
		}
		Comp->RegisterComponent();
		Owner->AddInstanceComponent(Comp);
		return Comp;
	}

	UTextRenderComponent* AddText(AActor* Owner, USceneComponent* Parent, const FString& Text, const FVector& Location,
		const FRotator& Rotation, float WorldSize, const FColor& Color)
	{
		if (!Owner)
		{
			return nullptr;
		}
		UTextRenderComponent* Comp = NewObject<UTextRenderComponent>(Owner);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetText(FText::FromString(Text));
		Comp->SetWorldSize(WorldSize);
		Comp->SetTextRenderColor(Color);
		Comp->SetHorizontalAlignment(EHTA_Center);
		Comp->SetVerticalAlignment(EVRTA_TextCenter);
		Comp->SetRelativeLocation(Location);
		Comp->SetRelativeRotation(Rotation);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Parent)
		{
			Comp->SetupAttachment(Parent);
		}
		Comp->RegisterComponent();
		Owner->AddInstanceComponent(Comp);
		return Comp;
	}

	UPointLightComponent* AddLight(AActor* Owner, USceneComponent* Parent, const FVector& Location, const FLinearColor& Color,
		float IntensityCandela, float RadiusCm)
	{
		if (!Owner)
		{
			return nullptr;
		}
		UPointLightComponent* Comp = NewObject<UPointLightComponent>(Owner);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->IntensityUnits = ELightUnits::Candelas;
		Comp->SetIntensity(IntensityCandela);
		Comp->SetLightColor(Color);
		Comp->SetAttenuationRadius(RadiusCm);
		Comp->SetCastShadows(false);
		Comp->SetRelativeLocation(Location);
		if (Parent)
		{
			Comp->SetupAttachment(Parent);
		}
		Comp->RegisterComponent();
		Owner->AddInstanceComponent(Comp);
		return Comp;
	}

	FTransform BoxTransform(const FVector& Center, const FVector& Size, const FRotator& Rotation)
	{
		return FTransform(Rotation, Center, Size / 100.0);
	}
}
