// Simulationskern: Fahrzeug-Datenbank, Elektrik (Akku, Controller, Motor, Kabel),
// Hitze/Durchbrennen, Laengsdynamik und Wheelie-Physik, Shop/Profil.
// Reines C++ ohne Unreal-Abhaengigkeiten, damit es auch ausserhalb der Engine
// getestet werden kann (siehe Tools/SimTest).
#pragma once

#if defined(SCOOTERSIM_STANDALONE)
	#include <cstdint>
	typedef int32_t int32;
	typedef uint32_t uint32;
	typedef uint8_t uint8;
	typedef wchar_t TCHAR;
	#ifndef TEXT
		#define TEXT(x) L##x
	#endif
#else
	#include "CoreMinimal.h"
#endif

namespace ScooterSim
{
	constexpr float Gravity = 9.81f;
	constexpr float AirDensity = 1.2f;
	constexpr float RiderMassKg = 80.f;
	constexpr float AmbientTempC = 22.f;
	constexpr float Pi = 3.14159265f;
	constexpr int32 MaxVehicles = 24;
	constexpr int32 MaxOptions = 32;

	inline float Clampf(float V, float Lo, float Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }
	inline float Minf(float A, float B) { return A < B ? A : B; }
	inline float Maxf(float A, float B) { return A > B ? A : B; }
	inline float Absf(float V) { return V < 0.f ? -V : V; }
	inline float Signf(float V) { return V > 0.f ? 1.f : (V < 0.f ? -1.f : 0.f); }
	inline float Lerpf(float A, float B, float T) { return A + (B - A) * T; }

	enum class EVehicleClass : uint8 { Scooter, Moto, Bicycle };
	enum class EVisualStyle : uint8 { CheapScooter, Scooter, HyperScooter, Moto, Bicycle };
	enum class EControllerKind : uint8 { Stock, Cheap, Vesc, Fardriver };
	enum class EMotorKind : uint8 { Hub, Mid };

	enum class EFailure : uint8
	{
		None,
		PhaseWire,
		BatteryWire,
		Connector,
		Controller,
		ControllerOvervoltage,
		Motor,
		BatteryFire
	};

	enum class EThermal : uint8 { Controller, Motor, Battery, PhaseWire, BatteryWire, Connector, Count };
	constexpr int32 NumThermal = (int32)EThermal::Count;

	// Teile-Kategorien im Shop. Kuehlungen haben nur die Optionen 0 (ohne) und 1 (mit).
	enum class EPart : uint8
	{
		Controller,
		Motor,
		PhaseWire,
		BatteryWire,
		Connector,
		Tire,
		Brake,
		Sprocket,
		MotorCooling,
		ControllerCooling,
		Count
	};
	constexpr int32 NumPartCategories = (int32)EPart::Count;

	struct FCellType
	{
		const TCHAR* Name;
		float CapacityAh;
		float ResistanceOhm;
		float MaxContA;
		float PricePerCell;
		float MassG;
	};

	struct FBmsType
	{
		const TCHAR* Name;
		float MaxA; // <= 0: Bypass, kein Schutz
		float Price;
	};

	struct FControllerDef
	{
		const TCHAR* Name;
		EControllerKind Kind;
		float MaxVoltage;
		float PeakPhaseA;
		float PeakBattA;
		float ContPhaseA;
		float MaxFieldWeakA;
		bool bAdjustable;
		bool bHasTempProtection;
		bool bInShop;
		float Price;
	};

	struct FMotorDef
	{
		const TCHAR* Name;
		EMotorKind Kind;
		float KvRpmPerV;
		float PhaseResistance;
		float ContPhaseA;
		float MassKg;
		float FwCharA;
		bool bInShop;
		float Price;
	};

	struct FSimplePart
	{
		const TCHAR* Name;
		float Value; // Kabel/Stecker: Dauerstrom A, Reifen: Grip, Bremse: Verzoegerung m/s^2, Ritzel: Zaehne
		float Price;
	};

	struct FVehicleDef
	{
		const TCHAR* Name;
		const TCHAR* Description;
		EVehicleClass Class;
		EVisualStyle Style;
		float Price;
		float MassKg;
		float WheelbaseM;
		float WheelRadiusM;
		float CgHeightM;
		float CgFromRearM;
		float LeanShiftM;
		float PullTorqueNm;
		float CdA;
		float RollingRes;
		int32 NumMotors;
		float GearRatio;
		int32 StockSprocketTeeth;
		float StockSpeedLimitKmh;
		int32 MaxCells;
		int32 MaxSeries;
		int32 MaxPhaseWire;
		int32 StockController;
		int32 StockMotor;
		int32 StockCell;
		int32 StockSeries;
		int32 StockParallel;
		int32 StockBms;
		int32 StockPhaseWire;
		int32 StockBattWire;
		int32 StockConnector;
		int32 StockTire;
		int32 StockBrake;
		uint32 ColorPrimary;
		uint32 ColorSecondary;
	};

	struct FThermalSpec
	{
		const TCHAR* Name;
		float RiseAtRatedK;
		float TimeConstS;
		float WarnC;
		float FailC;
		float AirflowCoef;
	};

	struct FPackInfo
	{
		int32 Series;
		int32 Parallel;
		int32 Cells;
		float NominalV;
		float FullV;
		float CapacityAh;
		float ResistanceOhm;
		float MaxContA;
		float BmsLimitA; // <= 0: Bypass
		float MassKg;
		float EnergyWh;
	};

	struct FVehicleSetup
	{
		int32 Parts[NumPartCategories];
		uint32 Owned[NumPartCategories];
		int32 Cell;
		int32 Series;
		int32 Parallel;
		int32 Bms;
		float PhaseA;
		float BattA;
		float FieldWeakA;
		bool bTempProtect;
		bool bSpeedLimit;
		bool bBrakeCutoff;
	};

	struct FVehicleState
	{
		float AhUsed;
		float Temp[NumThermal];
		EFailure Failure;
		float BmsCutTimer;
		float LastBattV;
	};

	struct FRideState
	{
		float Speed;      // m/s entlang der Fahrtrichtung
		float Pitch;      // Wheelie-Winkel relativ zum Boden (rad, >0 = Vorderrad oben)
		float PitchRate;
		float AirPitch;   // absoluter Nickwinkel in der Luft (rad)
		float AirPitchRate;
		float LastPropAccel;
	};

	struct FDriveInputs
	{
		float Throttle = 0.f;
		float Brake = 0.f;
		float Lean = 0.f;      // +1 = zuruecklehnen (Wheelie), -1 = nach vorne
		float SlopeSin = 0.f;
		float SlopeCos = 1.f;
		bool bGrounded = true;
		float Dt = 0.01f;
		float AirPathAngle = 0.f; // Steigwinkel der Flugbahn (rad), nur in der Luft
	};

	struct FTelemetry
	{
		float PhaseA;     // pro Motor
		float BattA;      // gesamt
		float BattV;
		float PowerW;
		float MotorRpm;
		float DriveForceN;
		float PropAccel;
		float FwBlend;
		bool bVoltageLimited;
		bool bBattLimited;
		bool bThermalDerate;
		bool bBmsCut;
		bool bBmsTempCut;
		bool bWheelspin;
		bool bSpeedLimited;
		bool bLoopOut;
		EFailure NewFailure;
	};

	struct FDerived
	{
		const FControllerDef* Ctrl;
		const FMotorDef* Motor;
		FPackInfo Pack;
		float TotalMassKg;
		float WheelRadius;
		float GearRatio;
		float DriveEff;
		float Ke;
		float Grip;
		float BrakeDecel;
		float PhaseWireA;
		float BattWireA;
		float ConnectorA;
		float ControllerCool;
		float MotorCool;
		float PhaseA;
		float BattA;
		float FieldWeakA;
		bool bTempProtect;
		bool bBrakeCutoff;
		float SpeedLimitMs; // <= 0: keine Drossel
		bool bOvervoltage;
	};

	struct FDynoResult
	{
		float TopSpeedKmh;
		float ZeroTo50S;   // < 0: nicht erreicht
		float PeakPowerW;
		float PeakPhaseA;
		float PeakBattA;
		float MaxTemp[NumThermal];
		EFailure Failure;
		float FailureTimeS;
		bool bBmsCut;
		bool bWouldWheelie;
	};

	struct FProfile
	{
		float Money;
		int32 Current;
		bool Owned[MaxVehicles];
		FVehicleSetup Setups[MaxVehicles];
		FVehicleState States[MaxVehicles];
		float BestTopSpeedKmh;
		float BestWheelieM;
		float TotalWheelieM;
		uint32 RacesWon;
		uint32 RadarMask;
		int32 Escapes;
		int32 Busts;
	};

	enum class EShopResult : uint8 { Bought, Installed, NotEnoughMoney, NotAvailable, AlreadyInstalled };

	// ---------- Datenbank ----------
	int32 NumVehicles();
	const FVehicleDef& Vehicle(int32 Index);
	int32 NumCells();
	const FCellType& Cell(int32 Index);
	int32 NumBms();
	const FBmsType& Bms(int32 Index);
	int32 NumControllers();
	const FControllerDef& Controller(int32 Index);
	int32 NumMotors();
	const FMotorDef& Motor(int32 Index);
	const FThermalSpec& ThermalSpec(EThermal Part);
	const TCHAR* FailureName(EFailure Failure);
	const TCHAR* PartCategoryName(EPart Category);

	// ---------- Shop-Optionen (generisch je Kategorie) ----------
	int32 NumOptions(EPart Category);
	const TCHAR* OptionName(EPart Category, int32 Index);
	float OptionValue(EPart Category, int32 Index);
	bool IsOptionAvailable(EPart Category, int32 Index, const FVehicleDef& Def);
	float OptionPrice(EPart Category, int32 Index, const FVehicleDef& Def);
	int32 StockOption(EPart Category, const FVehicleDef& Def);
	bool IsOwned(const FVehicleSetup& Setup, EPart Category, int32 Index);
	void SetOwned(FVehicleSetup& Setup, EPart Category, int32 Index, bool bOwned);

	// ---------- Akku ----------
	float CellOcv(float Soc);
	FPackInfo GetPackInfo(int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex);
	float StateOfCharge(const FPackInfo& Pack, float AhUsed);
	float BatteryPrice(int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex);
	bool IsBatteryValid(const FVehicleDef& Def, int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex);

	// ---------- Simulation ----------
	FDerived Derive(const FVehicleDef& Def, const FVehicleSetup& Setup);
	FVehicleState FreshState();
	void StepVehicle(const FVehicleDef& Def, const FDerived& D, FVehicleState& State, FRideState& Ride, const FDriveInputs& In, FTelemetry& Out);
	FDynoResult RunDyno(const FVehicleDef& Def, const FVehicleSetup& Setup, float Seconds);
	float BalancePointRad(const FVehicleDef& Def, float Lean);

	// ---------- Profil / Wirtschaft ----------
	FVehicleSetup MakeStockSetup(int32 VehicleIndex);
	void ApplyControllerDefaults(const FVehicleDef& Def, FVehicleSetup& Setup);
	void InitProfile(FProfile& Profile);
	EShopResult BuyVehicle(FProfile& Profile, int32 VehicleIndex);
	EShopResult BuyOrInstall(FProfile& Profile, int32 VehicleIndex, EPart Category, int32 OptionIndex);
	EShopResult BuildBattery(FProfile& Profile, int32 VehicleIndex, int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex);
	float RepairPrice(int32 VehicleIndex, const FVehicleSetup& Setup, EFailure Failure);
	EShopResult Repair(FProfile& Profile, int32 VehicleIndex);
	void CheapFix(FProfile& Profile, int32 VehicleIndex);
	void OnFailure(FProfile& Profile, int32 VehicleIndex, EFailure Failure);
}
