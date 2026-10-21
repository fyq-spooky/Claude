// Steuerung (Tastatur + Gamepad, ohne Input-Assets), Garage/Shop/Tuning-Menue,
// Geld, Spielstand und Benachrichtigungen.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ScooterSimCore.h"
#include "ScooterPlayerController.generated.h"

class AScooterPawn;
class AScooterWorld;
class APoliceManager;
class ARaceManager;

enum class EScooterMenu : uint8
{
	None,
	Garage,
	TuningApp
};

struct FScooterMenuRow
{
	FString Label;
	FString Value;
	FLinearColor Color = FLinearColor::White;
	bool bAdjustable = false;
};

struct FScooterInfoLine
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	bool bHeader = false;
};

struct FScooterNotice
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float TimeLeft = 0.f;
	float Duration = 1.f;
	bool bBig = false;
};

struct FScooterButtons
{
	bool Up = false;
	bool Down = false;
	bool Left = false;
	bool Right = false;
	bool Confirm = false;
	bool Back = false;
	bool Interact = false;
	bool Tuning = false;
	bool Reset = false;
	bool Help = false;
	bool Map = false;
	bool TabNext = false;
	bool TabPrev = false;
	bool Fine = false; // Shift gehalten: feinere Schritte
};

UCLASS()
class SCOOTERSIM_API AScooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AScooterPlayerController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	ScooterSim::FProfile& GetProfile();
	const ScooterSim::FProfile& GetProfileConst() const { return Profile; }
	AScooterPawn* GetScooter() const;
	AScooterWorld* GetCity() const;
	APoliceManager* GetPolice() const;
	ARaceManager* GetRaces() const;

	void Notify(const FString& Text, const FLinearColor& Color, float Duration = 3.f, bool bBig = false);
	void AddMoney(float Amount, const FString& Reason);
	bool SpendMoney(float Amount);
	void SaveProfile();

	void OnVehicleFailure(ScooterSim::EFailure Failure);
	void OnWheelieEnded(float DistM, float TimeS, float AvgSpeedMs);
	void OnStuntLanded(float AirTimeS, int32 Flips, float SpeedKmh);
	void OnCrash(const FString& Reason);
	void OnBusted(float Fine);

	// Fuer das HUD
	EScooterMenu GetMenu() const { return Menu; }
	const FString& GetMenuTitle() const { return MenuTitle; }
	const TArray<FString>& GetMenuTabs() const { return MenuTabs; }
	int32 GetMenuTabIndex() const { return MenuTabIndex; }
	int32 GetMenuRow() const { return MenuRow; }
	const TArray<FScooterMenuRow>& GetMenuRows() const { return MenuRows; }
	const TArray<FScooterInfoLine>& GetInfoLines() const { return InfoLines; }
	const TArray<FScooterNotice>& GetNotices() const { return Notices; }
	const FString& GetPrompt() const { return Prompt; }
	bool IsHelpVisible() const { return bShowHelp; }
	bool IsBigMap() const { return bBigMap; }

	static FString FormatMoney(float Amount);

private:
	void EnsureProfile();
	void LoadProfile();
	bool KeyDown(const FKey& Key) const;
	bool KeyEdge(const FKey& Key);
	bool VirtualEdge(FName Name, bool bDown);
	void ReadButtons(float Dt);
	void HandleDriving(float Dt, AScooterPawn* Scooter);
	void HandleWorldButtons(AScooterPawn* Scooter);
	void HandleMenuInput();
	void UpdateWarnings(float Dt, AScooterPawn* Scooter);
	void UpdatePrompt(AScooterPawn* Scooter);
	void UpdateNotices(float Dt);
	void TowToGarage();

	void OpenMenu(EScooterMenu InMenu, int32 Tab);
	void CloseMenu();
	void RefreshMenu();
	void AdjustRow(int32 Dir);
	void ConfirmRow();
	void ResetPreviews();
	TArray<int32> CurrentTabs() const;
	TArray<ScooterSim::EPart> PartCategoriesForVehicle() const;

	void BuildVehiclesTab();
	void BuildPartsTab();
	void BuildBatteryTab();
	void BuildSettingsTab();
	void BuildWorkshopTab();
	void AddInfo(const FString& Text, const FLinearColor& Color = FLinearColor::White, bool bHeader = false);
	void AddDynoInfo(const ScooterSim::FDynoResult& R, const FString& Title);
	void AddLimitsInfo();
	void ShowShopResult(ScooterSim::EShopResult Result, const FString& What, float Price);

	ScooterSim::FProfile Profile;
	bool bProfileReady = false;

	TMap<FName, bool> PrevKeys;
	FScooterButtons Btn;
	float RepeatUp = 0.f;
	float RepeatDown = 0.f;
	float RepeatLeft = 0.f;
	float RepeatRight = 0.f;
	float KeyThrottle = 0.f;
	float KeyBrake = 0.f;
	float KeySteer = 0.f;
	float KeyLean = 0.f;

	EScooterMenu Menu = EScooterMenu::None;
	FString MenuTitle;
	TArray<FString> MenuTabs;
	int32 MenuTabIndex = 0;
	int32 MenuTab = 0;
	int32 MenuRow = 0;
	TArray<FScooterMenuRow> MenuRows;
	TArray<FScooterInfoLine> InfoLines;
	TArray<int32> PartPreview;
	int32 BatCell = 0;
	int32 BatSeries = 10;
	int32 BatParallel = 2;
	int32 BatBms = 0;

	TArray<FScooterNotice> Notices;
	FString Prompt;
	bool bShowHelp = true;
	bool bBigMap = false;
	bool bSpeedTipShown = false;
	float HelpTimer = 25.f;
	TArray<float> WarnCooldown;
	float DerateCooldown = 0.f;
	float RecordCooldown = 0.f;
	float LastAnnouncedRecord = 0.f;
	float AutoSaveTimer = 60.f;
};
