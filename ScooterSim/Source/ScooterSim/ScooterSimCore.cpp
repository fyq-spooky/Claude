#include "ScooterSimCore.h"
#include <cmath>

namespace ScooterSim
{
	namespace
	{
		// Alle Werte ca. und fuer das Gameplay angepasst, keine offiziellen Herstellerangaben.

		const FCellType SSCells[] = {
			// Name                          Ah     R(Ohm)  MaxA  Preis  Gramm
			{ TEXT("NoName 18650 (Billigzelle)"), 2.0f, 0.080f, 8.f,  1.2f, 45.f },
			{ TEXT("Samsung 35E 18650"),          3.4f, 0.035f, 8.f,  3.5f, 50.f },
			{ TEXT("Samsung 40T 21700"),          4.0f, 0.013f, 30.f, 5.5f, 67.f },
			{ TEXT("Samsung 50S 21700"),          5.0f, 0.014f, 25.f, 6.5f, 72.f },
			{ TEXT("Molicel P45B 21700"),         4.5f, 0.009f, 45.f, 8.0f, 70.f },
			{ TEXT("Molicel P50B 21700"),         5.0f, 0.008f, 60.f, 10.f, 72.f },
		};

		const FBmsType SSBms[] = {
			{ TEXT("BMS 15A (billig)"), 15.f, 8.f },
			{ TEXT("BMS 30A"), 30.f, 15.f },
			{ TEXT("BMS 50A"), 50.f, 25.f },
			{ TEXT("BMS 80A"), 80.f, 40.f },
			{ TEXT("Smart-BMS 150A"), 150.f, 90.f },
			{ TEXT("Smart-BMS 250A"), 250.f, 150.f },
			{ TEXT("Smart-BMS 400A"), 400.f, 240.f },
			{ TEXT("BMS-Bypass (kein Schutz!)"), 0.f, 0.f },
		};

		const FControllerDef SSControllers[] = {
			// Name                                     Art                     MaxV   PeakPh  PeakBatt Dauer  FW    einst. Temp  Shop   Preis
			{ TEXT("Serien-Controller 36V 15A"),        EControllerKind::Stock, 45.f,  25.f,   15.f,    15.f,  0.f,  false, false, false, 25.f },
			{ TEXT("Serien-Controller 36V 18A"),        EControllerKind::Stock, 45.f,  32.f,   18.f,    18.f,  0.f,  false, false, false, 35.f },
			{ TEXT("Pedelec-Controller 36V 12A"),       EControllerKind::Stock, 45.f,  20.f,   12.f,    12.f,  0.f,  false, true,  false, 40.f },
			{ TEXT("Serien-Controller 48V 25A"),        EControllerKind::Stock, 60.f,  45.f,   25.f,    25.f,  0.f,  false, true,  false, 55.f },
			{ TEXT("Serien-Controller 48V 30A"),        EControllerKind::Stock, 60.f,  55.f,   30.f,    30.f,  0.f,  false, true,  false, 65.f },
			{ TEXT("Serien-Controller 52V 30A"),        EControllerKind::Stock, 63.f,  60.f,   30.f,    32.f,  0.f,  false, true,  false, 70.f },
			{ TEXT("Serien-Controller 60V 55A"),        EControllerKind::Stock, 72.f,  100.f,  55.f,    50.f,  0.f,  false, true,  false, 90.f },
			{ TEXT("Serien-Dual-Controller 52V 25A"),   EControllerKind::Stock, 63.f,  50.f,   25.f,    26.f,  0.f,  false, true,  false, 60.f },
			{ TEXT("Serien-Dual-Controller 72V 45A"),   EControllerKind::Stock, 90.f,  100.f,  45.f,    45.f,  0.f,  false, true,  false, 140.f },
			{ TEXT("Serien-Dual-Controller 84V 60A"),   EControllerKind::Stock, 100.f, 130.f,  60.f,    60.f,  0.f,  false, true,  false, 180.f },
			{ TEXT("Hyper-Serien-Controller 100V"),     EControllerKind::Stock, 110.f, 220.f,  110.f,   100.f, 0.f,  false, true,  false, 320.f },
			{ TEXT("Sur-Ron Serien-Controller"),        EControllerKind::Stock, 72.f,  260.f,  95.f,    110.f,  0.f,  false, true,  false, 450.f },
			{ TEXT("Talaria Serien-Controller"),        EControllerKind::Stock, 72.f,  360.f,  130.f,   140.f, 0.f,  false, true,  false, 520.f },
			{ TEXT("79Bike Serien-Controller"),         EControllerKind::Stock, 90.f,  450.f,  160.f,   180.f, 0.f,  false, true,  false, 600.f },
			{ TEXT("Ultra Bee Serien-Controller"),      EControllerKind::Stock, 90.f,  500.f,  175.f,   200.f, 0.f,  false, true,  false, 650.f },
			{ TEXT("Billig-Sinus-Controller 48V 35A"),  EControllerKind::Cheap, 60.f,  60.f,   35.f,    24.f,  0.f,  true,  false, true,  45.f },
			{ TEXT("Flipsky FSESC 75100 (VESC)"),       EControllerKind::Vesc,  75.f,  100.f,  80.f,    45.f,  40.f, true,  true,  true,  140.f },
			{ TEXT("Spintend Ubox 85150 (VESC)"),       EControllerKind::Vesc,  85.f,  150.f,  110.f,   65.f,  60.f, true,  true,  true,  260.f },
			{ TEXT("Makerbase 84200HP (VESC)"),         EControllerKind::Vesc,  84.f,  200.f,  150.f,   85.f,  80.f, true,  true,  true,  240.f },
			{ TEXT("3Shul C350 (VESC)"),                EControllerKind::Vesc,  90.f,  350.f,  250.f,   150.f, 120.f, true, true,  true,  650.f },
			{ TEXT("Fardriver ND72260"),                EControllerKind::Fardriver, 90.f,  260.f, 100.f, 105.f, 120.f, true, true, true, 230.f },
			{ TEXT("Fardriver ND72450"),                EControllerKind::Fardriver, 90.f,  450.f, 180.f, 170.f, 180.f, true, true, true, 390.f },
			{ TEXT("Fardriver ND72680"),                EControllerKind::Fardriver, 90.f,  680.f, 260.f, 240.f, 240.f, true, true, true, 540.f },
			{ TEXT("Fardriver ND96850"),                EControllerKind::Fardriver, 118.f, 850.f, 350.f, 300.f, 300.f, true, true, true, 820.f },
		};

		// EMotorKind::Hub = Roller-Nabenmotor, EMotorKind::Mid = Mittelmotor (E-Moto).
		// Fahrrad-Nabenmotoren werden ueber den Namen "E-Bike" und die Fahrzeugklasse zugeordnet.
		const FMotorDef SSMotors[] = {
			// Name                                    Art              Kv     R(Ohm)   Dauer  kg     FW-Kenn Shop   Preis
			{ TEXT("Serien-Nabenmotor 250W"),          EMotorKind::Hub, 21.5f, 0.26f,   12.f,  3.0f,  40.f,  false, 40.f },
			{ TEXT("Serien-Nabenmotor 350W"),          EMotorKind::Hub, 23.f,  0.20f,   15.f,  3.5f,  45.f,  false, 50.f },
			{ TEXT("E-Bike Serien-Nabenmotor 250W"),   EMotorKind::Hub, 8.1f,  0.35f,   10.f,  3.2f,  30.f,  false, 60.f },
			{ TEXT("Serien-Nabenmotor 600W"),          EMotorKind::Hub, 21.7f, 0.15f,   22.f,  4.2f,  55.f,  false, 70.f },
			{ TEXT("Serien-Nabenmotor 1000W"),         EMotorKind::Hub, 26.5f, 0.12f,   30.f,  5.0f,  60.f,  false, 90.f },
			{ TEXT("Serien-Nabenmotor 1200W"),         EMotorKind::Hub, 22.f,  0.11f,   32.f,  5.3f,  60.f,  false, 100.f },
			{ TEXT("Serien-Nabenmotor 2000W"),         EMotorKind::Hub, 24.5f, 0.08f,   42.f,  6.5f,  70.f,  false, 150.f },
			{ TEXT("Serien-Nabenmotor 1000W (Dual)"),  EMotorKind::Hub, 29.f,  0.12f,   28.f,  5.0f,  60.f,  false, 90.f },
			{ TEXT("Serien-Nabenmotor 2700W (Dual)"),  EMotorKind::Hub, 27.f,  0.07f,   48.f,  7.0f,  75.f,  false, 200.f },
			{ TEXT("Serien-Nabenmotor 4000W (Dual)"),  EMotorKind::Hub, 26.f,  0.05f,   65.f,  9.0f,  85.f,  false, 260.f },
			{ TEXT("Hyper-Nabenmotor 5000W"),          EMotorKind::Hub, 31.f,  0.035f,  95.f,  11.f,  100.f, false, 520.f },
			{ TEXT("Sur-Ron Serienmotor"),             EMotorKind::Mid, 95.f,  0.020f,  85.f,  11.f,  160.f, false, 500.f },
			{ TEXT("Talaria Serienmotor"),             EMotorKind::Mid, 100.f, 0.016f,  105.f, 12.f,  170.f, false, 560.f },
			{ TEXT("79Bike Serienmotor"),              EMotorKind::Mid, 86.f,  0.013f,  140.f, 14.f,  190.f, false, 700.f },
			{ TEXT("Ultra Bee Serienmotor"),           EMotorKind::Mid, 75.f,  0.012f,  150.f, 15.f,  200.f, false, 800.f },
			{ TEXT("Nabenmotor 1600W"),                EMotorKind::Hub, 25.f,  0.09f,   40.f,  6.0f,  70.f,  true,  180.f },
			{ TEXT("Nabenmotor 2500W"),                EMotorKind::Hub, 27.f,  0.065f,  55.f,  7.3f,  80.f,  true,  280.f },
			{ TEXT("Nabenmotor 3500W Torque-Wicklung"),EMotorKind::Hub, 23.f,  0.055f,  70.f,  8.5f,  90.f,  true,  420.f },
			{ TEXT("Nabenmotor 3500W Speed-Wicklung"), EMotorKind::Hub, 34.f,  0.045f,  70.f,  8.5f,  90.f,  true,  420.f },
			{ TEXT("Nabenmotor 5000W Hyper"),          EMotorKind::Hub, 30.f,  0.033f,  100.f, 11.f,  100.f, true,  700.f },
			{ TEXT("Nabenmotor 7000W Race"),           EMotorKind::Hub, 33.f,  0.024f,  140.f, 13.5f, 120.f, true,  1150.f },
			{ TEXT("QS138 70H Mittelmotor"),           EMotorKind::Mid, 98.f,  0.014f,  140.f, 13.f,  180.f, true,  650.f },
			{ TEXT("QS138 90H Mittelmotor"),           EMotorKind::Mid, 90.f,  0.010f,  200.f, 16.f,  220.f, true,  950.f },
			{ TEXT("QS165 V3 Mittelmotor"),            EMotorKind::Mid, 80.f,  0.0075f, 290.f, 22.f,  260.f, true,  1600.f },
			{ TEXT("E-Bike Nabenmotor 1000W Kit"),     EMotorKind::Hub, 9.5f,  0.15f,   25.f,  5.5f,  60.f,  true,  190.f },
			{ TEXT("E-Bike Nabenmotor 3000W Kit"),     EMotorKind::Hub, 10.5f, 0.06f,   55.f,  8.5f,  90.f,  true,  350.f },
			{ TEXT("E-Bike Nabenmotor 5000W Kit"),     EMotorKind::Hub, 11.f,  0.04f,   80.f,  11.f,  110.f, true,  520.f },
		};

		const FSimplePart SSWires[] = {
			{ TEXT("18 AWG"), 20.f, 3.f },
			{ TEXT("16 AWG"), 30.f, 5.f },
			{ TEXT("14 AWG"), 42.f, 8.f },
			{ TEXT("12 AWG"), 60.f, 12.f },
			{ TEXT("10 AWG"), 90.f, 18.f },
			{ TEXT("8 AWG"), 135.f, 28.f },
			{ TEXT("6 AWG"), 190.f, 40.f },
			{ TEXT("4 AWG"), 270.f, 60.f },
			{ TEXT("2 AWG"), 380.f, 90.f },
		};

		const FSimplePart SSConnectors[] = {
			{ TEXT("XT30"), 30.f, 2.f },
			{ TEXT("XT60"), 60.f, 3.f },
			{ TEXT("XT90"), 90.f, 5.f },
			{ TEXT("AS150"), 150.f, 9.f },
			{ TEXT("QS8 Anti-Spark"), 200.f, 14.f },
			{ TEXT("QS10"), 300.f, 22.f },
			{ TEXT("Anderson SB350"), 380.f, 35.f },
		};

		const FSimplePart SSTires[] = {
			{ TEXT("Billig-Reifen"), 0.75f, 0.f },
			{ TEXT("Standard-Reifen"), 0.90f, 25.f },
			{ TEXT("Sportreifen"), 1.05f, 70.f },
			{ TEXT("Semi-Slicks"), 1.20f, 130.f },
			{ TEXT("Stollenreifen (Offroad)"), 0.85f, 60.f },
			{ TEXT("Supermoto-Reifen"), 1.15f, 150.f },
		};

		const FSimplePart SSBrakes[] = {
			{ TEXT("Trommelbremse"), 4.0f, 0.f },
			{ TEXT("Mech. Scheibenbremse"), 6.0f, 40.f },
			{ TEXT("Hydraulische Scheibenbremse"), 8.0f, 110.f },
			{ TEXT("4-Kolben-Hydraulik"), 9.5f, 240.f },
		};

		const FSimplePart SSSprockets[] = {
			{ TEXT("Ritzel 42Z (Topspeed)"), 42.f, 25.f },
			{ TEXT("Ritzel 48Z (Serie)"), 48.f, 25.f },
			{ TEXT("Ritzel 52Z"), 52.f, 25.f },
			{ TEXT("Ritzel 58Z (Wheelie)"), 58.f, 30.f },
			{ TEXT("Ritzel 64Z"), 64.f, 30.f },
			{ TEXT("Ritzel 70Z (Monster-Wheelie)"), 70.f, 35.f },
		};

		const FSimplePart SSMotorCooling[] = {
			{ TEXT("Keine Motork\u00fchlung"), 1.f, 0.f },
			{ TEXT("Ferrofluid / Motorl\u00fcfter"), 1.45f, 45.f },
		};

		const FSimplePart SSControllerCooling[] = {
			{ TEXT("Keine Controllerk\u00fchlung"), 1.f, 0.f },
			{ TEXT("K\u00fchlk\u00f6rper + L\u00fcfter"), 1.6f, 40.f },
		};

		const FVehicleDef SSVehicles[] = {
			{ TEXT("Discounter-Roller \"Flitzer\""), TEXT("250W Billigroller. Langsam, schwach, perfekt zum Basteln."),
			  EVehicleClass::Scooter, EVisualStyle::CheapScooter, 150.f, 12.f, 0.82f, 0.108f, 1.02f, 0.43f, 0.12f, 25.f, 0.62f, 0.016f,
			  1, 1.f, 0, 20.f, 30, 13, 2,   0, 0, 0, 10, 2, 0,   0, 1, 0, 0, 0,   0xE8E8E8, 0xFF3B7F },
			{ TEXT("Klapp-Roller \"Blitz 350\""), TEXT("350W Pendlerroller mit 25 km/h Drossel."),
			  EVehicleClass::Scooter, EVisualStyle::CheapScooter, 350.f, 14.f, 0.90f, 0.127f, 1.0f, 0.47f, 0.12f, 30.f, 0.62f, 0.014f,
			  1, 1.f, 0, 25.f, 40, 13, 3,   1, 1, 1, 10, 3, 1,   1, 1, 1, 1, 1,   0x2B2B2B, 0x33C3FF },
			{ TEXT("Baumarkt-Pedelec"), TEXT("28 Zoll Pedelec, 250W, 25 km/h. Mit Kit wird daraus eine Rakete."),
			  EVehicleClass::Bicycle, EVisualStyle::Bicycle, 400.f, 25.f, 1.12f, 0.35f, 1.05f, 0.48f, 0.15f, 40.f, 0.55f, 0.008f,
			  1, 1.f, 0, 25.f, 52, 14, 3,   2, 2, 1, 10, 3, 0,   1, 1, 1, 1, 0,   0x3A7D44, 0xF2F2F2 },
			{ TEXT("Kukirin G2 Pro"), TEXT("48V 600W Allrounder, entdrosselt ca. 45 km/h."),
			  EVehicleClass::Scooter, EVisualStyle::Scooter, 550.f, 21.f, 1.00f, 0.127f, 1.0f, 0.50f, 0.12f, 35.f, 0.62f, 0.013f,
			  1, 1.f, 0, 25.f, 78, 16, 4,   3, 3, 1, 13, 4, 1,   2, 2, 1, 1, 1,   0x1E1E1E, 0xFFD400 },
			{ TEXT("Kukirin G2 Max"), TEXT("48V 1000W, gro\u00dfer Akku, ca. 55 km/h."),
			  EVehicleClass::Scooter, EVisualStyle::Scooter, 750.f, 24.f, 1.02f, 0.127f, 1.0f, 0.51f, 0.12f, 35.f, 0.62f, 0.013f,
			  1, 1.f, 0, 25.f, 90, 16, 4,   4, 4, 1, 13, 6, 2,   3, 2, 1, 1, 1,   0x1E1E1E, 0xFF8C00 },
			{ TEXT("Ausom Leopard"), TEXT("48V 1000W mit Sportreifen und Hydraulikbremsen."),
			  EVehicleClass::Scooter, EVisualStyle::Scooter, 800.f, 25.f, 1.03f, 0.127f, 1.0f, 0.51f, 0.12f, 35.f, 0.62f, 0.013f,
			  1, 1.f, 0, 25.f, 90, 16, 4,   4, 4, 1, 13, 6, 2,   3, 2, 2, 2, 2,   0x0F4C81, 0xE0E0E0 },
			{ TEXT("Kukirin G3"), TEXT("52V 1200W, ca. 50 km/h, stabiler Rahmen."),
			  EVehicleClass::Scooter, EVisualStyle::Scooter, 900.f, 26.f, 1.05f, 0.135f, 1.0f, 0.52f, 0.12f, 40.f, 0.62f, 0.013f,
			  1, 1.f, 0, 25.f, 98, 18, 4,   5, 5, 1, 14, 5, 2,   3, 2, 2, 1, 1,   0x262626, 0x00C2A8 },
			{ TEXT("Kukirin G4"), TEXT("60V 2000W Offroad-Roller, ca. 75 km/h."),
			  EVehicleClass::Scooter, EVisualStyle::Scooter, 1300.f, 34.f, 1.12f, 0.152f, 1.02f, 0.55f, 0.12f, 40.f, 0.64f, 0.014f,
			  1, 1.f, 0, 25.f, 128, 20, 5,  6, 6, 1, 16, 7, 3,   4, 3, 2, 1, 2,   0x1A1A1A, 0xFF2D2D },
			{ TEXT("Ausom Gallop"), TEXT("52V Dual-Motor 2x1000W, ca. 65 km/h."),
			  EVehicleClass::Scooter, EVisualStyle::Scooter, 1400.f, 32.f, 1.10f, 0.127f, 1.0f, 0.54f, 0.12f, 40.f, 0.64f, 0.013f,
			  2, 1.f, 0, 25.f, 112, 18, 4,  7, 7, 1, 14, 6, 3,   2, 3, 2, 2, 2,   0x0F4C81, 0xFFB000 },
			{ TEXT("Dualtron Thunder 2"), TEXT("72V Dual-Motor, ca. 95 km/h. Der Klassiker."),
			  EVehicleClass::Scooter, EVisualStyle::HyperScooter, 3800.f, 46.f, 1.18f, 0.14f, 1.0f, 0.58f, 0.12f, 45.f, 0.62f, 0.013f,
			  2, 1.f, 0, 0.f, 260, 24, 5,   8, 8, 1, 20, 12, 4,  4, 5, 3, 2, 2,   0x111111, 0xFFC700 },
			{ TEXT("Sur-Ron Light Bee X"), TEXT("60V Mittelmotor E-Moto, ca. 75 km/h, Wheelie-Maschine."),
			  EVehicleClass::Moto, EVisualStyle::Moto, 3900.f, 52.f, 1.26f, 0.33f, 0.82f, 0.58f, 0.20f, 80.f, 0.50f, 0.020f,
			  1, 9.5f, 48, 0.f, 200, 24, 8,  11, 11, 2, 16, 8, 4,  6, 5, 3, 4, 2,   0x2A2A2A, 0x9BE000 },
			{ TEXT("Talaria Sting"), TEXT("60V E-Moto, mehr Strom als die Light Bee, ca. 85 km/h."),
			  EVehicleClass::Moto, EVisualStyle::Moto, 4500.f, 60.f, 1.28f, 0.33f, 0.83f, 0.58f, 0.20f, 85.f, 0.50f, 0.020f,
			  1, 9.0f, 48, 0.f, 220, 24, 8,  12, 12, 4, 16, 9, 4,  7, 6, 4, 4, 2,   0xDDDDDD, 0xD7261E },
			{ TEXT("79Bike Falcon Pro"), TEXT("72V E-Moto mit Supermoto-Reifen, ca. 95 km/h."),
			  EVehicleClass::Moto, EVisualStyle::Moto, 5500.f, 68.f, 1.30f, 0.34f, 0.84f, 0.60f, 0.20f, 90.f, 0.48f, 0.018f,
			  1, 8.6f, 48, 0.f, 260, 26, 8,  13, 13, 4, 20, 9, 5,  8, 6, 4, 5, 3,   0x101820, 0x00A3FF },
			{ TEXT("Sur-Ron Ultra Bee"), TEXT("74V E-Moto, viel Drehmoment, ca. 90 km/h."),
			  EVehicleClass::Moto, EVisualStyle::Moto, 6500.f, 85.f, 1.36f, 0.35f, 0.86f, 0.62f, 0.20f, 95.f, 0.52f, 0.020f,
			  1, 8.0f, 48, 0.f, 300, 26, 8,  14, 14, 3, 20, 11, 5, 8, 6, 4, 4, 3,   0x3B3B3B, 0x9BE000 },
			{ TEXT("Dualtron X Limited"), TEXT("84V Dual-Motor Panzer, 75 kg, ca. 110 km/h."),
			  EVehicleClass::Scooter, EVisualStyle::HyperScooter, 7000.f, 75.f, 1.32f, 0.165f, 1.0f, 0.66f, 0.12f, 50.f, 0.66f, 0.013f,
			  2, 1.f, 0, 0.f, 380, 26, 6,   9, 9, 2, 23, 15, 4,  5, 6, 4, 2, 3,   0x0B0B0B, 0xFFC700 },
			{ TEXT("Hyper-Roller \"Vortex\""), TEXT("100V Hyperscooter, 2x5000W, ca. 130 km/h. Endgegner-Material."),
			  EVehicleClass::Scooter, EVisualStyle::HyperScooter, 9000.f, 68.f, 1.30f, 0.155f, 0.98f, 0.64f, 0.12f, 50.f, 0.55f, 0.012f,
			  2, 1.f, 0, 0.f, 300, 28, 7,   10, 10, 4, 24, 10, 5, 6, 7, 5, 3, 3,  0x1B0033, 0xC000FF },
		};

		const FThermalSpec SSThermal[NumThermal] = {
			// Name             Anstieg  Tau    Warnung Defekt  Fahrtwind
			{ TEXT("Controller"),   48.f, 28.f,  85.f,  120.f, 0.030f },
			{ TEXT("Motor"),        70.f, 80.f,  120.f, 165.f, 0.035f },
			{ TEXT("Akku"),         22.f, 140.f, 55.f,  85.f,  0.010f },
			{ TEXT("Phasenkabel"),  65.f, 5.f,   120.f, 190.f, 0.015f },
			{ TEXT("Akkukabel"),    65.f, 6.f,   120.f, 190.f, 0.015f },
			{ TEXT("Stecker"),      55.f, 10.f,  100.f, 150.f, 0.012f },
		};

		template <typename T, int N>
		constexpr int32 SSCountOf(const T (&)[N]) { return N; }

		template <typename T, int N>
		const T& SSAt(const T (&Arr)[N], int32 Index)
		{
			const int32 Safe = Index < 0 ? 0 : (Index >= N ? N - 1 : Index);
			return Arr[Safe];
		}

		const FSimplePart* SSSimpleTable(EPart Category, int32& OutCount)
		{
			switch (Category)
			{
			case EPart::PhaseWire:
			case EPart::BatteryWire: OutCount = SSCountOf(SSWires); return SSWires;
			case EPart::Connector: OutCount = SSCountOf(SSConnectors); return SSConnectors;
			case EPart::Tire: OutCount = SSCountOf(SSTires); return SSTires;
			case EPart::Brake: OutCount = SSCountOf(SSBrakes); return SSBrakes;
			case EPart::Sprocket: OutCount = SSCountOf(SSSprockets); return SSSprockets;
			case EPart::MotorCooling: OutCount = SSCountOf(SSMotorCooling); return SSMotorCooling;
			case EPart::ControllerCooling: OutCount = SSCountOf(SSControllerCooling); return SSControllerCooling;
			default: OutCount = 0; return nullptr;
			}
		}

		bool SSIsBikeMotor(const FMotorDef& M)
		{
			// "E-Bike" am Namensanfang kennzeichnet Fahrrad-Nabenmotoren
			const TCHAR* N = M.Name;
			return N[0] == 'E' && N[1] == '-' && N[2] == 'B';
		}

		int32 SSStockSprocket(const FVehicleDef& Def)
		{
			for (int32 i = 0; i < SSCountOf(SSSprockets); ++i)
			{
				if ((int32)SSSprockets[i].Value == Def.StockSprocketTeeth)
				{
					return i;
				}
			}
			return 1;
		}

		void SSHeat(FVehicleState& St, EThermal Part, float Ratio, float CoolMul, float AbsSpeed, float Dt)
		{
			const FThermalSpec& Spec = SSThermal[(int32)Part];
			const float Cool = CoolMul * (1.f + Spec.AirflowCoef * AbsSpeed);
			const float Target = AmbientTempC + Spec.RiseAtRatedK * Ratio * Ratio / Cool;
			const float K = 1.f - std::exp(-Dt / Spec.TimeConstS);
			float& T = St.Temp[(int32)Part];
			T += (Target - T) * K;
		}
	}

	// ---------------------------------------------------------------------------------------------
	// Datenbank

	int32 NumVehicles() { return SSCountOf(SSVehicles); }
	const FVehicleDef& Vehicle(int32 Index) { return SSAt(SSVehicles, Index); }
	int32 NumCells() { return SSCountOf(SSCells); }
	const FCellType& Cell(int32 Index) { return SSAt(SSCells, Index); }
	int32 NumBms() { return SSCountOf(SSBms); }
	const FBmsType& Bms(int32 Index) { return SSAt(SSBms, Index); }
	int32 NumControllers() { return SSCountOf(SSControllers); }
	const FControllerDef& Controller(int32 Index) { return SSAt(SSControllers, Index); }
	int32 NumMotors() { return SSCountOf(SSMotors); }
	const FMotorDef& Motor(int32 Index) { return SSAt(SSMotors, Index); }
	const FThermalSpec& ThermalSpec(EThermal Part) { return SSThermal[(int32)Part]; }

	const TCHAR* FailureName(EFailure Failure)
	{
		switch (Failure)
		{
		case EFailure::PhaseWire: return TEXT("Phasenkabel durchgebrannt");
		case EFailure::BatteryWire: return TEXT("Akkukabel geschmolzen");
		case EFailure::Connector: return TEXT("Akkustecker geschmolzen");
		case EFailure::Controller: return TEXT("Controller abgeraucht (MOSFETs durch)");
		case EFailure::ControllerOvervoltage: return TEXT("Controller durch \u00dcberspannung geplatzt");
		case EFailure::Motor: return TEXT("Motorwicklung verbrannt");
		case EFailure::BatteryFire: return TEXT("AKKUBRAND! Zellen thermisch durchgegangen");
		default: return TEXT("Alles ok");
		}
	}

	const TCHAR* PartCategoryName(EPart Category)
	{
		switch (Category)
		{
		case EPart::Controller: return TEXT("Controller");
		case EPart::Motor: return TEXT("Motor");
		case EPart::PhaseWire: return TEXT("Phasenkabel");
		case EPart::BatteryWire: return TEXT("Akkukabel");
		case EPart::Connector: return TEXT("Akkustecker");
		case EPart::Tire: return TEXT("Reifen");
		case EPart::Brake: return TEXT("Bremsen");
		case EPart::Sprocket: return TEXT("Ritzel / \u00dcbersetzung");
		case EPart::MotorCooling: return TEXT("Motork\u00fchlung");
		case EPart::ControllerCooling: return TEXT("Controllerk\u00fchlung");
		default: return TEXT("?");
		}
	}

	// ---------------------------------------------------------------------------------------------
	// Shop-Optionen

	int32 NumOptions(EPart Category)
	{
		if (Category == EPart::Controller) return NumControllers();
		if (Category == EPart::Motor) return NumMotors();
		int32 Count = 0;
		SSSimpleTable(Category, Count);
		return Count;
	}

	const TCHAR* OptionName(EPart Category, int32 Index)
	{
		if (Category == EPart::Controller) return Controller(Index).Name;
		if (Category == EPart::Motor) return Motor(Index).Name;
		int32 Count = 0;
		const FSimplePart* Table = SSSimpleTable(Category, Count);
		if (!Table || Count == 0) return TEXT("?");
		return Table[Index < 0 ? 0 : (Index >= Count ? Count - 1 : Index)].Name;
	}

	float OptionValue(EPart Category, int32 Index)
	{
		int32 Count = 0;
		const FSimplePart* Table = SSSimpleTable(Category, Count);
		if (!Table || Count == 0) return 0.f;
		return Table[Index < 0 ? 0 : (Index >= Count ? Count - 1 : Index)].Value;
	}

	int32 StockOption(EPart Category, const FVehicleDef& Def)
	{
		switch (Category)
		{
		case EPart::Controller: return Def.StockController;
		case EPart::Motor: return Def.StockMotor;
		case EPart::PhaseWire: return Def.StockPhaseWire;
		case EPart::BatteryWire: return Def.StockBattWire;
		case EPart::Connector: return Def.StockConnector;
		case EPart::Tire: return Def.StockTire;
		case EPart::Brake: return Def.StockBrake;
		case EPart::Sprocket: return SSStockSprocket(Def);
		default: return 0;
		}
	}

	bool IsOptionAvailable(EPart Category, int32 Index, const FVehicleDef& Def)
	{
		if (Index < 0 || Index >= NumOptions(Category)) return false;
		const bool bStock = Index == StockOption(Category, Def);
		switch (Category)
		{
		case EPart::Controller:
			return bStock || Controller(Index).bInShop;
		case EPart::Motor:
		{
			const FMotorDef& M = Motor(Index);
			const FMotorDef& StockM = Motor(Def.StockMotor);
			if (bStock) return true;
			if (!M.bInShop || M.Kind != StockM.Kind) return false;
			if (M.Kind == EMotorKind::Hub)
			{
				// Fahrrad bekommt nur E-Bike-Naben, Roller nur Roller-Naben
				return SSIsBikeMotor(M) == (Def.Class == EVehicleClass::Bicycle);
			}
			return true;
		}
		case EPart::PhaseWire:
			// Bei Nabenmotoren laufen die Phasenkabel durch die Achse: begrenzter Querschnitt
			return Index <= Def.MaxPhaseWire;
		case EPart::Sprocket:
			return Def.Class == EVehicleClass::Moto;
		case EPart::Tire:
			if (Def.Class == EVehicleClass::Moto) return Index >= 2;
			return Index != 4 && Index != 5;
		default:
			return true;
		}
	}

	float OptionPrice(EPart Category, int32 Index, const FVehicleDef& Def)
	{
		const float PerMotor = (float)Def.NumMotors;
		switch (Category)
		{
		case EPart::Controller: return Controller(Index).Price * PerMotor;
		case EPart::Motor: return Motor(Index).Price * PerMotor;
		case EPart::PhaseWire:
		{
			int32 Count = 0;
			const FSimplePart* Table = SSSimpleTable(Category, Count);
			return Table[Index < 0 ? 0 : (Index >= Count ? Count - 1 : Index)].Price * PerMotor;
		}
		default:
		{
			int32 Count = 0;
			const FSimplePart* Table = SSSimpleTable(Category, Count);
			if (!Table || Count == 0) return 0.f;
			return Table[Index < 0 ? 0 : (Index >= Count ? Count - 1 : Index)].Price;
		}
		}
	}

	bool IsOwned(const FVehicleSetup& Setup, EPart Category, int32 Index)
	{
		if (Index < 0 || Index >= MaxOptions) return false;
		return (Setup.Owned[(int32)Category] & (1u << (uint32)Index)) != 0;
	}

	void SetOwned(FVehicleSetup& Setup, EPart Category, int32 Index, bool bOwned)
	{
		if (Index < 0 || Index >= MaxOptions) return;
		const uint32 Bit = 1u << (uint32)Index;
		if (bOwned) Setup.Owned[(int32)Category] |= Bit;
		else Setup.Owned[(int32)Category] &= ~Bit;
	}

	// ---------------------------------------------------------------------------------------------
	// Akku

	float CellOcv(float Soc)
	{
		static const float SocPts[] = { 0.f, 0.05f, 0.10f, 0.20f, 0.30f, 0.50f, 0.70f, 0.90f, 1.0f };
		static const float VPts[] = { 3.0f, 3.30f, 3.45f, 3.55f, 3.62f, 3.72f, 3.87f, 4.05f, 4.2f };
		const float S = Clampf(Soc, 0.f, 1.f);
		for (int32 i = 1; i < 9; ++i)
		{
			if (S <= SocPts[i])
			{
				const float T = (S - SocPts[i - 1]) / (SocPts[i] - SocPts[i - 1]);
				return Lerpf(VPts[i - 1], VPts[i], T);
			}
		}
		return 4.2f;
	}

	FPackInfo GetPackInfo(int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex)
	{
		const FCellType& C = Cell(CellIndex);
		FPackInfo P;
		P.Series = Series < 1 ? 1 : Series;
		P.Parallel = Parallel < 1 ? 1 : Parallel;
		P.Cells = P.Series * P.Parallel;
		P.NominalV = 3.65f * (float)P.Series;
		P.FullV = 4.2f * (float)P.Series;
		P.CapacityAh = C.CapacityAh * (float)P.Parallel;
		// Zellwiderstand plus etwas fuer Zellverbinder und BMS
		P.ResistanceOhm = C.ResistanceOhm * (float)P.Series / (float)P.Parallel + 0.004f;
		P.MaxContA = C.MaxContA * (float)P.Parallel;
		P.BmsLimitA = Bms(BmsIndex).MaxA;
		P.MassKg = (float)P.Cells * C.MassG / 1000.f * 1.25f;
		P.EnergyWh = P.NominalV * P.CapacityAh;
		return P;
	}

	float StateOfCharge(const FPackInfo& Pack, float AhUsed)
	{
		return Clampf(1.f - AhUsed / Maxf(Pack.CapacityAh, 0.01f), 0.f, 1.f);
	}

	float BatteryPrice(int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex)
	{
		return (float)(Series * Parallel) * Cell(CellIndex).PricePerCell + Bms(BmsIndex).Price + 25.f;
	}

	bool IsBatteryValid(const FVehicleDef& Def, int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex)
	{
		if (CellIndex < 0 || CellIndex >= NumCells()) return false;
		if (BmsIndex < 0 || BmsIndex >= NumBms()) return false;
		if (Series < 7 || Series > Def.MaxSeries) return false;
		if (Parallel < 1 || Series * Parallel > Def.MaxCells) return false;
		return true;
	}

	// ---------------------------------------------------------------------------------------------
	// Simulation

	FDerived Derive(const FVehicleDef& Def, const FVehicleSetup& Setup)
	{
		FDerived D;
		D.Ctrl = &Controller(Setup.Parts[(int32)EPart::Controller]);
		D.Motor = &Motor(Setup.Parts[(int32)EPart::Motor]);
		D.Pack = GetPackInfo(Setup.Cell, Setup.Series, Setup.Parallel, Setup.Bms);

		const FPackInfo StockPack = GetPackInfo(Def.StockCell, Def.StockSeries, Def.StockParallel, Def.StockBms);
		const FMotorDef& StockMotor = Motor(Def.StockMotor);
		D.TotalMassKg = Def.MassKg + RiderMassKg + (D.Pack.MassKg - StockPack.MassKg)
			+ (D.Motor->MassKg - StockMotor.MassKg) * (float)Def.NumMotors;
		D.TotalMassKg = Maxf(D.TotalMassKg, Def.MassKg * 0.5f + RiderMassKg);

		D.WheelRadius = Def.WheelRadiusM;
		D.GearRatio = Def.GearRatio;
		if (Def.Class == EVehicleClass::Moto && Def.StockSprocketTeeth > 0)
		{
			D.GearRatio *= OptionValue(EPart::Sprocket, Setup.Parts[(int32)EPart::Sprocket]) / (float)Def.StockSprocketTeeth;
		}
		D.DriveEff = (D.Motor->Kind == EMotorKind::Hub) ? 0.97f : 0.92f;
		D.Ke = 60.f / (2.f * Pi * D.Motor->KvRpmPerV);
		D.Grip = OptionValue(EPart::Tire, Setup.Parts[(int32)EPart::Tire]);
		D.BrakeDecel = OptionValue(EPart::Brake, Setup.Parts[(int32)EPart::Brake]);
		D.PhaseWireA = OptionValue(EPart::PhaseWire, Setup.Parts[(int32)EPart::PhaseWire]);
		D.BattWireA = OptionValue(EPart::BatteryWire, Setup.Parts[(int32)EPart::BatteryWire]);
		D.ConnectorA = OptionValue(EPart::Connector, Setup.Parts[(int32)EPart::Connector]);
		D.ControllerCool = OptionValue(EPart::ControllerCooling, Setup.Parts[(int32)EPart::ControllerCooling]);
		D.MotorCool = OptionValue(EPart::MotorCooling, Setup.Parts[(int32)EPart::MotorCooling]);

		const FControllerDef& C = *D.Ctrl;
		if (C.bAdjustable)
		{
			D.PhaseA = Clampf(Setup.PhaseA, 5.f, C.PeakPhaseA);
			D.BattA = Clampf(Setup.BattA, 3.f, C.PeakBattA);
			D.FieldWeakA = Clampf(Setup.FieldWeakA, 0.f, C.MaxFieldWeakA);
			D.bTempProtect = Setup.bTempProtect && C.bHasTempProtection;
		}
		else
		{
			D.PhaseA = C.PeakPhaseA;
			D.BattA = C.PeakBattA;
			D.FieldWeakA = 0.f;
			D.bTempProtect = C.bHasTempProtection;
		}
		D.bBrakeCutoff = Setup.bBrakeCutoff;
		D.SpeedLimitMs = (Setup.bSpeedLimit && Def.StockSpeedLimitKmh > 0.f) ? Def.StockSpeedLimitKmh / 3.6f : 0.f;
		D.bOvervoltage = D.Pack.FullV > C.MaxVoltage + 0.01f;
		return D;
	}

	FVehicleState FreshState()
	{
		FVehicleState S;
		S.AhUsed = 0.f;
		for (int32 i = 0; i < NumThermal; ++i)
		{
			S.Temp[i] = AmbientTempC;
		}
		S.Failure = EFailure::None;
		S.BmsCutTimer = 0.f;
		S.LastBattV = 0.f;
		return S;
	}

	float BalancePointRad(const FVehicleDef& Def, float Lean)
	{
		const float Lr = Def.CgFromRearM - Clampf(Lean, -1.f, 1.f) * Def.LeanShiftM;
		return std::atan2(Lr, Def.CgHeightM);
	}

	void StepVehicle(const FVehicleDef& Def, const FDerived& D, FVehicleState& St, FRideState& Ride, const FDriveInputs& In, FTelemetry& Out)
	{
		Out = FTelemetry();
		Out.NewFailure = EFailure::None;

		const float Dt = In.Dt;
		const float Mass = D.TotalMassKg;
		const float AbsV = Absf(Ride.Speed);
		const float MotorCount = (float)Def.NumMotors;
		const float Throttle = Clampf(In.Throttle, 0.f, 1.f);
		const float BrakeIn = Clampf(In.Brake, 0.f, 1.f);
		const float Lean = Clampf(In.Lean, -1.f, 1.f);

		if (St.BmsCutTimer > 0.f)
		{
			St.BmsCutTimer = Maxf(0.f, St.BmsCutTimer - Dt);
		}

		// Ueberspannung: Controller platzt beim Einschalten
		if (D.bOvervoltage && St.Failure == EFailure::None)
		{
			St.Failure = EFailure::ControllerOvervoltage;
			Out.NewFailure = St.Failure;
		}

		const float Soc = StateOfCharge(D.Pack, St.AhUsed);
		const float Voc = (float)D.Pack.Series * CellOcv(Soc);
		float Vb = (St.LastBattV > 1.f) ? Minf(St.LastBattV, Voc) : Voc;

		const bool bHasBms = D.Pack.BmsLimitA > 0.f;
		if (bHasBms && St.Temp[(int32)EThermal::Battery] > 60.f)
		{
			St.BmsCutTimer = Maxf(St.BmsCutTimer, 0.3f);
			Out.bBmsTempCut = true;
		}

		const bool bPowerOn = St.Failure == EFailure::None && St.BmsCutTimer <= 0.f;
		const float MotorOmega = AbsV / D.WheelRadius * D.GearRatio;
		Out.MotorRpm = MotorOmega * 60.f / (2.f * Pi);

		float Iq = 0.f;
		float Id = 0.f;
		float IbPerMotor = 0.f;

		// Bremsabschaltung: Bremshebel gezogen = Motor aus (abschaltbar fuer Wheelie-Profis)
		const bool bBrakeCut = D.bBrakeCutoff && BrakeIn > 0.1f;
		if (bPowerOn && !bBrakeCut && Throttle > 0.001f && Ride.Speed > -0.5f)
		{
			float Cmd = Throttle * D.PhaseA;
			if (!In.bGrounded)
			{
				Cmd = Minf(Cmd, 6.f);
			}
			if (D.bTempProtect)
			{
				const float CtrlT = St.Temp[(int32)EThermal::Controller];
				const float MotorT = St.Temp[(int32)EThermal::Motor];
				const float Derate = Minf(1.f - Clampf((CtrlT - 85.f) / 25.f, 0.f, 1.f), 1.f - Clampf((MotorT - 125.f) / 30.f, 0.f, 1.f));
				if (Derate < 0.99f)
				{
					Out.bThermalDerate = true;
				}
				Cmd *= Derate;
			}
			if (D.SpeedLimitMs > 0.f)
			{
				const float Lim = Clampf((D.SpeedLimitMs - AbsV) / 0.8f, 0.f, 1.f);
				if (Lim < 0.99f)
				{
					Out.bSpeedLimited = true;
				}
				Cmd *= Lim;
			}
			const float CutV = (float)D.Pack.Series * 3.05f;
			Cmd *= Clampf((Vb - CutV) / 2.f, 0.f, 1.f);

			const float E = MotorOmega * D.Ke;
			const float R = D.Motor->PhaseResistance;
			bool bVLimited = false;
			bool bBLimited = false;
			for (int32 Iter = 0; Iter < 4; ++Iter)
			{
				const float Vmax = Vb * 0.95f;
				float EffE = E;
				Id = 0.f;
				if (D.FieldWeakA > 0.f)
				{
					const float Blend = Clampf((E / Maxf(Vmax, 1.f) - 0.82f) / 0.18f, 0.f, 1.f);
					Id = D.FieldWeakA * Blend * Throttle;
					const float Strength = 0.35f * Id / (Id + D.Motor->FwCharA);
					EffE = E * (1.f - Strength);
					Out.FwBlend = Blend;
				}
				Iq = Cmd;
				bVLimited = false;
				bBLimited = false;
				if (EffE + Iq * R > Vmax)
				{
					Iq = Maxf(0.f, (Vmax - EffE) / R);
					bVLimited = true;
				}
				const float FwLoss = Id * Id * R * 1.5f;
				float Pm = (EffE + Iq * R) * Iq + FwLoss;
				const float PbMax = D.BattA * Vb * 0.96f;
				if (Pm > PbMax)
				{
					const float Avail = Maxf(0.f, PbMax - FwLoss);
					Iq = (-EffE + std::sqrt(EffE * EffE + 4.f * R * Avail)) / (2.f * R);
					Pm = (EffE + Iq * R) * Iq + FwLoss;
					bBLimited = true;
				}
				IbPerMotor = Pm / Maxf(Vb * 0.96f, 1.f);
				Vb = Maxf(1.f, Voc - IbPerMotor * MotorCount * D.Pack.ResistanceOhm);
			}
			Out.bVoltageLimited = bVLimited;
			Out.bBattLimited = bBLimited;

			if (bHasBms && IbPerMotor * MotorCount > D.Pack.BmsLimitA * 1.001f)
			{
				St.BmsCutTimer = 1.2f;
				Out.bBmsCut = true;
				Iq = 0.f;
				Id = 0.f;
				IbPerMotor = 0.f;
				Vb = Voc;
			}
		}

		const float BattTotal = IbPerMotor * MotorCount;
		St.LastBattV = (BattTotal > 0.f) ? Vb : Voc;
		St.AhUsed += BattTotal * Dt / 3600.f;

		const float PhaseMag = std::sqrt(Iq * Iq + Id * Id);
		Out.PhaseA = PhaseMag;
		Out.BattA = BattTotal;
		Out.BattV = St.LastBattV;
		Out.PowerW = BattTotal * St.LastBattV;

		// ---- Hitze
		SSHeat(St, EThermal::Controller, PhaseMag / D.Ctrl->ContPhaseA, D.ControllerCool, AbsV, Dt);
		SSHeat(St, EThermal::Motor, PhaseMag / D.Motor->ContPhaseA, D.MotorCool, AbsV, Dt);
		SSHeat(St, EThermal::Battery, BattTotal / Maxf(D.Pack.MaxContA, 1.f), 1.f, AbsV, Dt);
		SSHeat(St, EThermal::PhaseWire, PhaseMag / D.PhaseWireA, 1.f, AbsV, Dt);
		SSHeat(St, EThermal::BatteryWire, BattTotal / D.BattWireA, 1.f, AbsV, Dt);
		SSHeat(St, EThermal::Connector, BattTotal / D.ConnectorA, 1.f, AbsV, Dt);

		if (St.Failure == EFailure::None)
		{
			struct FCheck { EThermal Part; EFailure Fail; };
			const FCheck Checks[] = {
				{ EThermal::PhaseWire, EFailure::PhaseWire },
				{ EThermal::BatteryWire, EFailure::BatteryWire },
				{ EThermal::Connector, EFailure::Connector },
				{ EThermal::Controller, EFailure::Controller },
				{ EThermal::Motor, EFailure::Motor },
				{ EThermal::Battery, EFailure::BatteryFire },
			};
			for (const FCheck& C : Checks)
			{
				if (St.Temp[(int32)C.Part] >= SSThermal[(int32)C.Part].FailC)
				{
					St.Failure = C.Fail;
					Out.NewFailure = C.Fail;
					break;
				}
			}
		}

		// ---- Kraefte
		float DriveForce = D.Ke * Iq * D.GearRatio / D.WheelRadius * D.DriveEff * MotorCount;
		if (!In.bGrounded)
		{
			DriveForce = 0.f;
		}

		const float Lr = Def.CgFromRearM - Lean * Def.LeanShiftM;
		const float H = Def.CgHeightM;
		const float L = Def.WheelbaseM;
		const float NTotal = Mass * Gravity * In.SlopeCos;
		const bool bWheelie = Ride.Pitch > 0.005f;
		const float NRear = bWheelie ? NTotal : Clampf(NTotal * (L - Lr) / L + Mass * Ride.LastPropAccel * H / L, 0.f, NTotal);
		const float NFront = NTotal - NRear;
		const float Mu = D.Grip;

		float FRear = DriveForce * ((Def.NumMotors > 1) ? 0.5f : 1.f);
		float FFront = DriveForce - FRear;
		if (bWheelie)
		{
			FFront = 0.f;
		}
		if (FRear > Mu * NRear)
		{
			FRear = Mu * NRear * 0.85f;
			Out.bWheelspin = true;
		}
		if (FFront > Mu * NFront)
		{
			FFront = Mu * NFront * 0.85f;
			Out.bWheelspin = true;
		}
		DriveForce = FRear + FFront;
		Out.DriveForceN = DriveForce;

		const float V = Ride.Speed;
		const float Drag = 0.5f * AirDensity * Def.CdA * V * AbsV;
		const float BrakeDecel = BrakeIn * Minf(D.BrakeDecel, Mu * Gravity) * (bWheelie ? 0.5f : 1.f);
		const float FrictionDecel = BrakeDecel + Def.RollingRes * Gravity * In.SlopeCos;

		// Vortriebsbeschleunigung ohne Hangabtrieb (fuer das Nickmoment)
		float PropAccel = (DriveForce - Drag) / Mass;
		if (AbsV > 0.05f)
		{
			PropAccel -= Signf(V) * FrictionDecel;
		}
		Out.PropAccel = PropAccel;

		if (In.bGrounded)
		{
			float NewV = V + ((DriveForce - Drag) / Mass - Gravity * In.SlopeSin) * Dt;
			const float FrictionDv = FrictionDecel * Dt;
			if (NewV > FrictionDv) NewV -= FrictionDv;
			else if (NewV < -FrictionDv) NewV += FrictionDv;
			else NewV = 0.f;
			Ride.Speed = NewV;
			Ride.LastPropAccel = PropAccel;

			// ---- Wheelie: Drehung um den Hinterrad-Aufstandspunkt
			const float Theta = Ride.Pitch;
			const float X = Lr * std::cos(Theta) - H * std::sin(Theta);
			const float Z = Lr * std::sin(Theta) + H * std::cos(Theta);
			const float Inertia = Mass * (Lr * Lr + H * H) * 0.85f;
			float Torque = Mass * PropAccel * Z - Mass * Gravity * In.SlopeCos * X;
			if (Lean > 0.f && Theta < 0.5f)
			{
				Torque += Def.PullTorqueNm * Lean;
			}
			const float Alpha = Torque / Inertia - 2.2f * Ride.PitchRate;
			Ride.PitchRate += Alpha * Dt;
			Ride.Pitch += Ride.PitchRate * Dt;
			if (Ride.Pitch <= 0.f)
			{
				Ride.Pitch = 0.f;
				if (Ride.PitchRate < 0.f)
				{
					Ride.PitchRate = 0.f;
				}
			}
			const float Balance = std::atan2(Lr, H);
			if (Ride.Pitch > Balance + 0.30f || Ride.Pitch > 1.35f)
			{
				Out.bLoopOut = true;
			}
		}
		else
		{
			// In der Luft: Koerpergewicht, Gas (Rad dreht hoch) und Bremse (Rad stoppt) drehen das Fahrzeug.
			// Ohne Lehnen richtet sich das Fahrzeug langsam an der Flugbahn aus (wie ein echter Fahrer).
			const float Align = 1.f - Absf(Lean);
			const float AirAlpha = Lean * 3.0f + Throttle * 1.0f - BrakeIn * 2.2f
				+ Align * (6.f * (In.AirPathAngle - Ride.AirPitch) - 2.5f * Ride.AirPitchRate);
			Ride.AirPitchRate += AirAlpha * Dt;
			Ride.AirPitchRate *= (1.f - 0.4f * Dt);
			Ride.AirPitch += Ride.AirPitchRate * Dt;
			Ride.LastPropAccel = 0.f;
		}
	}

	FDynoResult RunDyno(const FVehicleDef& Def, const FVehicleSetup& Setup, float Seconds)
	{
		FDynoResult R;
		R.TopSpeedKmh = 0.f;
		R.ZeroTo50S = -1.f;
		R.PeakPowerW = 0.f;
		R.PeakPhaseA = 0.f;
		R.PeakBattA = 0.f;
		for (int32 i = 0; i < NumThermal; ++i)
		{
			R.MaxTemp[i] = AmbientTempC;
		}
		R.Failure = EFailure::None;
		R.FailureTimeS = -1.f;
		R.bBmsCut = false;
		R.bWouldWheelie = false;

		const FDerived D = Derive(Def, Setup);
		FVehicleState St = FreshState();
		FRideState Ride = FRideState();
		FDriveInputs In;
		In.Throttle = 1.f;
		In.Brake = 0.f;
		In.Lean = 0.f;
		In.SlopeSin = 0.f;
		In.SlopeCos = 1.f;
		In.bGrounded = true;
		In.Dt = 0.01f;

		const int32 Steps = (int32)(Seconds / In.Dt);
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			FTelemetry Out;
			StepVehicle(Def, D, St, Ride, In, Out);
			const float T = (float)(Step + 1) * In.Dt;
			if (Ride.Pitch > 0.02f)
			{
				R.bWouldWheelie = true;
			}
			// Pruefstand: Vorderrad wird festgehalten
			Ride.Pitch = 0.f;
			Ride.PitchRate = 0.f;

			const float Kmh = Ride.Speed * 3.6f;
			R.TopSpeedKmh = Maxf(R.TopSpeedKmh, Kmh);
			if (R.ZeroTo50S < 0.f && Kmh >= 50.f)
			{
				R.ZeroTo50S = T;
			}
			R.PeakPowerW = Maxf(R.PeakPowerW, Out.PowerW);
			R.PeakPhaseA = Maxf(R.PeakPhaseA, Out.PhaseA);
			R.PeakBattA = Maxf(R.PeakBattA, Out.BattA);
			for (int32 i = 0; i < NumThermal; ++i)
			{
				R.MaxTemp[i] = Maxf(R.MaxTemp[i], St.Temp[i]);
			}
			if (Out.bBmsCut)
			{
				R.bBmsCut = true;
			}
			if (Out.NewFailure != EFailure::None)
			{
				R.Failure = Out.NewFailure;
				R.FailureTimeS = T;
				break;
			}
		}
		return R;
	}

	// ---------------------------------------------------------------------------------------------
	// Profil / Wirtschaft

	void ApplyControllerDefaults(const FVehicleDef& Def, FVehicleSetup& Setup)
	{
		const FControllerDef& C = Controller(Setup.Parts[(int32)EPart::Controller]);
		if (!C.bAdjustable)
		{
			Setup.PhaseA = C.PeakPhaseA;
			Setup.BattA = C.PeakBattA;
			Setup.FieldWeakA = 0.f;
			Setup.bTempProtect = C.bHasTempProtection;
			return;
		}
		// Die Werkstatt stellt erst mal "sicher" ein. Mehr ist Sache des Spielers.
		const float PhaseWireA = OptionValue(EPart::PhaseWire, Setup.Parts[(int32)EPart::PhaseWire]);
		const float BattWireA = OptionValue(EPart::BatteryWire, Setup.Parts[(int32)EPart::BatteryWire]);
		const float ConnA = OptionValue(EPart::Connector, Setup.Parts[(int32)EPart::Connector]);
		const FPackInfo Pack = GetPackInfo(Setup.Cell, Setup.Series, Setup.Parallel, Setup.Bms);
		float BattLimit = Minf(BattWireA, ConnA);
		BattLimit = Minf(BattLimit, Pack.MaxContA);
		if (Pack.BmsLimitA > 0.f)
		{
			BattLimit = Minf(BattLimit, Pack.BmsLimitA);
		}
		BattLimit /= (float)Def.NumMotors;
		Setup.PhaseA = Clampf(Minf(C.PeakPhaseA * 0.5f, PhaseWireA * 1.3f), 5.f, C.PeakPhaseA);
		Setup.BattA = Clampf(Minf(C.PeakBattA * 0.5f, BattLimit * 0.9f), 3.f, C.PeakBattA);
		Setup.FieldWeakA = 0.f;
		Setup.bTempProtect = C.bHasTempProtection;
	}

	FVehicleSetup MakeStockSetup(int32 VehicleIndex)
	{
		const FVehicleDef& Def = Vehicle(VehicleIndex);
		FVehicleSetup S;
		for (int32 c = 0; c < NumPartCategories; ++c)
		{
			const EPart Cat = (EPart)c;
			S.Parts[c] = StockOption(Cat, Def);
			S.Owned[c] = 0u;
			SetOwned(S, Cat, S.Parts[c], true);
		}
		S.Cell = Def.StockCell;
		S.Series = Def.StockSeries;
		S.Parallel = Def.StockParallel;
		S.Bms = Def.StockBms;
		S.bSpeedLimit = Def.StockSpeedLimitKmh > 0.f;
		S.PhaseA = 0.f;
		S.BattA = 0.f;
		S.FieldWeakA = 0.f;
		S.bTempProtect = true;
		S.bBrakeCutoff = true;
		ApplyControllerDefaults(Def, S);
		return S;
	}

	void InitProfile(FProfile& Profile)
	{
		Profile.Money = 300.f;
		Profile.Current = 0;
		for (int32 i = 0; i < MaxVehicles; ++i)
		{
			Profile.Owned[i] = false;
			Profile.Setups[i] = MakeStockSetup(i < NumVehicles() ? i : 0);
			Profile.States[i] = FreshState();
		}
		Profile.Owned[0] = true;
		Profile.BestTopSpeedKmh = 0.f;
		Profile.BestWheelieM = 0.f;
		Profile.TotalWheelieM = 0.f;
		Profile.RacesWon = 0u;
		Profile.RadarMask = 0u;
		Profile.Escapes = 0;
		Profile.Busts = 0;
	}

	EShopResult BuyVehicle(FProfile& Profile, int32 VehicleIndex)
	{
		if (VehicleIndex < 0 || VehicleIndex >= NumVehicles()) return EShopResult::NotAvailable;
		if (Profile.Owned[VehicleIndex])
		{
			if (Profile.Current == VehicleIndex) return EShopResult::AlreadyInstalled;
			Profile.Current = VehicleIndex;
			return EShopResult::Installed;
		}
		const float Price = Vehicle(VehicleIndex).Price;
		if (Profile.Money < Price) return EShopResult::NotEnoughMoney;
		Profile.Money -= Price;
		Profile.Owned[VehicleIndex] = true;
		Profile.Setups[VehicleIndex] = MakeStockSetup(VehicleIndex);
		Profile.States[VehicleIndex] = FreshState();
		Profile.Current = VehicleIndex;
		return EShopResult::Bought;
	}

	EShopResult BuyOrInstall(FProfile& Profile, int32 VehicleIndex, EPart Category, int32 OptionIndex)
	{
		const FVehicleDef& Def = Vehicle(VehicleIndex);
		FVehicleSetup& S = Profile.Setups[VehicleIndex];
		if (!IsOptionAvailable(Category, OptionIndex, Def)) return EShopResult::NotAvailable;
		const int32 Cat = (int32)Category;
		if (S.Parts[Cat] == OptionIndex) return EShopResult::AlreadyInstalled;

		EShopResult Result = EShopResult::Installed;
		if (!IsOwned(S, Category, OptionIndex))
		{
			const float Price = OptionPrice(Category, OptionIndex, Def);
			if (Profile.Money < Price) return EShopResult::NotEnoughMoney;
			Profile.Money -= Price;
			SetOwned(S, Category, OptionIndex, true);
			Result = EShopResult::Bought;
		}
		S.Parts[Cat] = OptionIndex;
		if (Category == EPart::Controller)
		{
			ApplyControllerDefaults(Def, S);
		}
		return Result;
	}

	EShopResult BuildBattery(FProfile& Profile, int32 VehicleIndex, int32 CellIndex, int32 Series, int32 Parallel, int32 BmsIndex)
	{
		const FVehicleDef& Def = Vehicle(VehicleIndex);
		FVehicleSetup& S = Profile.Setups[VehicleIndex];
		if (!IsBatteryValid(Def, CellIndex, Series, Parallel, BmsIndex)) return EShopResult::NotAvailable;
		if (S.Cell == CellIndex && S.Series == Series && S.Parallel == Parallel && S.Bms == BmsIndex) return EShopResult::AlreadyInstalled;
		const float Price = BatteryPrice(CellIndex, Series, Parallel, BmsIndex);
		if (Profile.Money < Price) return EShopResult::NotEnoughMoney;
		Profile.Money -= Price;
		S.Cell = CellIndex;
		S.Series = Series;
		S.Parallel = Parallel;
		S.Bms = BmsIndex;
		FVehicleState& St = Profile.States[VehicleIndex];
		St.AhUsed = 0.f;
		St.LastBattV = 0.f;
		St.Temp[(int32)EThermal::Battery] = AmbientTempC;
		if (St.Failure == EFailure::BatteryFire)
		{
			St.Failure = EFailure::None;
		}
		return EShopResult::Bought;
	}

	namespace
	{
		EPart SSFailurePart(EFailure Failure)
		{
			switch (Failure)
			{
			case EFailure::PhaseWire: return EPart::PhaseWire;
			case EFailure::BatteryWire: return EPart::BatteryWire;
			case EFailure::Connector: return EPart::Connector;
			case EFailure::Controller:
			case EFailure::ControllerOvervoltage: return EPart::Controller;
			case EFailure::Motor: return EPart::Motor;
			default: return EPart::Count;
			}
		}
	}

	float RepairPrice(int32 VehicleIndex, const FVehicleSetup& Setup, EFailure Failure)
	{
		const FVehicleDef& Def = Vehicle(VehicleIndex);
		switch (Failure)
		{
		case EFailure::None: return 0.f;
		case EFailure::PhaseWire: return OptionPrice(EPart::PhaseWire, Setup.Parts[(int32)EPart::PhaseWire], Def) + 15.f;
		case EFailure::BatteryWire: return OptionPrice(EPart::BatteryWire, Setup.Parts[(int32)EPart::BatteryWire], Def) + 15.f;
		case EFailure::Connector: return OptionPrice(EPart::Connector, Setup.Parts[(int32)EPart::Connector], Def) + 10.f;
		case EFailure::Controller:
		case EFailure::ControllerOvervoltage: return OptionPrice(EPart::Controller, Setup.Parts[(int32)EPart::Controller], Def);
		case EFailure::Motor: return OptionPrice(EPart::Motor, Setup.Parts[(int32)EPart::Motor], Def) * 0.6f + 40.f;
		case EFailure::BatteryFire: return BatteryPrice(Setup.Cell, Setup.Series, Setup.Parallel, Setup.Bms) + 80.f;
		default: return 0.f;
		}
	}

	void OnFailure(FProfile& Profile, int32 VehicleIndex, EFailure Failure)
	{
		// Das kaputte Teil ist weg (ausser Serienteil, das gibt's beim Haendler immer gratis nach)
		const EPart Part = SSFailurePart(Failure);
		if (Part == EPart::Count) return;
		const FVehicleDef& Def = Vehicle(VehicleIndex);
		FVehicleSetup& S = Profile.Setups[VehicleIndex];
		const int32 Installed = S.Parts[(int32)Part];
		if (Installed != StockOption(Part, Def))
		{
			SetOwned(S, Part, Installed, false);
		}
	}

	EShopResult Repair(FProfile& Profile, int32 VehicleIndex)
	{
		FVehicleState& St = Profile.States[VehicleIndex];
		if (St.Failure == EFailure::None) return EShopResult::AlreadyInstalled;
		FVehicleSetup& S = Profile.Setups[VehicleIndex];
		const float Price = RepairPrice(VehicleIndex, S, St.Failure);
		if (Profile.Money < Price) return EShopResult::NotEnoughMoney;
		Profile.Money -= Price;
		const EPart Part = SSFailurePart(St.Failure);
		if (Part != EPart::Count)
		{
			SetOwned(S, Part, S.Parts[(int32)Part], true);
		}
		const float AhUsed = St.AhUsed;
		St = FreshState();
		St.AhUsed = (Part == EPart::Count) ? 0.f : AhUsed;
		return EShopResult::Bought;
	}

	void CheapFix(FProfile& Profile, int32 VehicleIndex)
	{
		FVehicleState& St = Profile.States[VehicleIndex];
		if (St.Failure == EFailure::None) return;
		const FVehicleDef& Def = Vehicle(VehicleIndex);
		FVehicleSetup& S = Profile.Setups[VehicleIndex];
		const EPart Part = SSFailurePart(St.Failure);
		if (Part != EPart::Count)
		{
			S.Parts[(int32)Part] = StockOption(Part, Def);
			if (Part == EPart::Controller)
			{
				ApplyControllerDefaults(Def, S);
			}
		}
		else if (St.Failure == EFailure::BatteryFire)
		{
			S.Cell = Def.StockCell;
			S.Series = Def.StockSeries;
			S.Parallel = Def.StockParallel;
			S.Bms = Def.StockBms;
		}
		const float AhUsed = St.AhUsed;
		St = FreshState();
		St.AhUsed = (Part == EPart::Count) ? 0.f : AhUsed;
	}
}
