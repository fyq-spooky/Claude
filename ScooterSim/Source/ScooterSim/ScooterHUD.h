// Komplettes HUD per Canvas (keine Widget-Assets noetig): Tacho, Elektrik, Temperaturen,
// Wheelie-Meter, Minimap, Polizei, Rennen, Meldungen, Hilfe und das Garagen-Menue.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ScooterHUD.generated.h"

class AScooterPlayerController;
class AScooterPawn;
class UFont;

UCLASS()
class SCOOTERSIM_API AScooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void Txt(const FString& Text, float X, float Y, float Px, const FLinearColor& Color, int32 Align = 0);
	float TxtW(const FString& Text, float Px) const;
	void Rect(float X, float Y, float W, float H, const FLinearColor& Color);
	void Bar(float X, float Y, float W, float H, float Frac, const FLinearColor& Fill);
	void Ln(float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Thick);
	void Panel(float X, float Y, float W, float H);

	void DrawMenu(AScooterPlayerController* PC);
	void DrawDriving(AScooterPlayerController* PC, AScooterPawn* Pawn);
	void DrawMap(AScooterPlayerController* PC, AScooterPawn* Pawn, bool bBig);
	void DrawPoliceAndRace(AScooterPlayerController* PC, AScooterPawn* Pawn);
	void DrawNotices(AScooterPlayerController* PC);
	void DrawHelp();
	static bool ClipSegment(FVector2D& A, FVector2D& B, const FVector2D& Min, const FVector2D& Max);

	UFont* HudFont = nullptr;
	float Ui = 1.f;
	float DesignW = 1920.f;
	float BaseCharH = 20.f;
};
