# ScooterSim: E-Scooter & E-Moto Tuning-Simulator (Unreal Engine 5)

Kauf dir Roller und E-Motos, bau Controller, Akkus und Kabel um, jag die Leistung hoch und pass auf, dass nichts durchbrennt. Mach Wheelies, spring über Rampen, fahr Rennen und häng die Polizei ab. Ziel: das **schnellste Fahrzeug der Stadt**.

Die ganze Welt (Stadt, Rampen, Polizei, Rennen, HUD, Sound) wird beim Start **per C++ erzeugt**. Du musst im Editor nichts bauen oder zusammenklicken: Projekt öffnen, Play drücken, fahren.

---

## Installation (einmalig)

1. **Unreal Engine 5** über den [Epic Games Launcher](https://www.unrealengine.com/download) installieren (5.3 oder neuer).
2. **Visual Studio 2022** (kostenlose Community-Version) installieren und dabei die Arbeitslast **„Spieleentwicklung mit C++"** anhaken.
3. Dieses Repository herunterladen: auf GitHub **Code → Download ZIP** und entpacken (oder `git clone`).

## Starten

1. Doppelklick auf **`ScooterSim/ScooterSim.uproject`**.
2. Fragt Unreal *„The following modules are missing or built with a different engine version… Would you like to rebuild them now?"* → **Ja**. Das Kompilieren dauert beim ersten Mal ein paar Minuten.
3. Im Editor oben auf **Play** (▶) klicken, dann einmal ins Spielfenster klicken.

> **Andere Engine-Version?** Rechtsklick auf `ScooterSim.uproject` → *Switch Unreal Engine version…* → deine Version auswählen.
> **Kompilierfehler?** Schick mir die Fehlermeldung aus dem Fenster bzw. aus `ScooterSim/Saved/Logs`, dann fixe ich das.

---

## Steuerung

| Aktion | Tastatur | Gamepad |
|---|---|---|
| Gas | W / Pfeil hoch | RT |
| Bremse (im Stand halten = schieben) | S / Pfeil runter | LT |
| Lenken | A / D | linker Stick |
| **Zurücklehnen = Wheelie** | **Leertaste** | rechter Stick runter |
| Nach vorne lehnen | Strg / C | rechter Stick hoch |
| Garage öffnen / Rennen starten | E | A |
| Tuning-App (im Stand, überall) | T | Y |
| Zurücksetzen / Abschleppen (30 €) | R | X |
| Karte | M | Back |
| Hilfe ein/aus | H | Start |

**Im Menü:** W/S = Auswahl, A/D = Teil/Wert ändern (Shift = feine Schritte), Enter = kaufen/einbauen, Tab/Q = Reiter wechseln, Backspace = schließen.

---

## Was drin ist

### Fahrzeuge (16 Stück)
Billigroller, Pendlerroller, Baumarkt-Pedelec, **Kukirin G2 Pro / G2 Max / G3 / G4**, **Ausom Leopard / Gallop**, **Dualtron Thunder 2 / X Limited**, Hyper-Roller „Vortex", **Sur-Ron Light Bee X / Ultra Bee**, **Talaria Sting**, **79Bike Falcon Pro**.
Du startest mit 300 € und einem 20-km/h-Billigroller.

### Tuning mit echter Elektrik-Simulation
- **Controller:** Serie, Billig-Sinus, **VESC** (Flipsky, Spintend, Makerbase, 3Shul) und **Fardriver** (ND72260 bis ND96850)
- **Einstellungen:** Phasenstrom, Akkustrom, **Feldschwächung**, Temperaturschutz, Bremsabschaltung, Drossel
- **Akku-Baukasten:** Zelltyp (NoName bis Molicel P50B), Reihe (S), Parallel (P), BMS oder **BMS-Bypass**
- **Teile:** Nabenmotoren / QS-Mittelmotoren, Phasen- und Akkukabel (18 bis 2 AWG), Stecker (XT30 bis Anderson), Reifen, Bremsen, Ritzel (E-Motos), Ferrofluid / Controller-Kühlung
- Ein **Prüfstand** im Menü zeigt vor dem Kauf Topspeed, Beschleunigung, Leistung und die Temperaturen jedes Teils.

### Was durchbrennen kann 🔥
| Zu viel … | Folge |
|---|---|
| Phasenstrom für die Phasenkabel | **Phasenkabel brennen durch** |
| Akkustrom für Akkukabel / Stecker | **Kabel schmelzen / Stecker schmilzt** |
| Strom ohne Kühlung / ohne Temperaturschutz | **Controller raucht ab, Motorwicklung verbrennt** |
| Akkuspannung (S × 4,2 V) über Controller-Maximum | **Controller platzt sofort beim Losfahren** |
| BMS-Bypass + schwache Zellen + viel Strom | **Akkubrand** |

Mit BMS schaltet der Akku bei Überstrom ab (ruckelt), statt zu brennen. Reparieren geht in der Werkstatt: Originalteil kaufen oder gratis das Serienteil wieder einbauen.

### Wheelie-System
Echte Physik ums Hinterrad: Beschleunigung, Schwerpunkt, Zurücklehnen und Bremse. Halte die Balance (Anzeige rechts), sonst überschlägst du dich. Wheelies, Airtime und **Backflips** bringen Geld. Starke E-Motos steigen leicht, Roller erst mit viel Tuning. Tipp: Bremsabschaltung aus, dann kannst du den Wheelie mit der Bremse fangen.

### Rennen & Geld
7 Rennen (Drag 200 m bis 2 km, Stadtkurse) gegen Gegner, die mit derselben Physik fahren, dazu 2 Wheelie-Challenges und eine **Topspeed-Radarfalle** auf dem Highway mit Belohnungen bei 60, 100, 140, 180 und 220 km/h. Endgegner: der „Highway-König" mit ca. 173 km/h.

### Polizei
Über 30 km/h jagt dich jede Streife, die dich sieht. Blitzer rufen sie auch. Das Fahndungslevel steigt mit Tempo und Dauer. Häng sie ab, indem du aus der Sicht verschwindest (durch **Parks und Gassen**: Poller lassen Roller durch, Autos nicht) und kassier eine Belohnung. Wenn sie dich schnappen, zahlst du Strafe.

---

## Für Bastler

```
ScooterSim/
  ScooterSim.uproject
  Config/                  Engine-, Spiel- und Input-Einstellungen
  Source/ScooterSim/
    ScooterSimCore.*       Simulationskern: Fahrzeugdaten, Elektrik, Hitze, Wheelie, Shop (reines C++)
    ScooterPawn.*          Fahrphysik, Rampen/Sprünge, Crashes, Effekte
    ScooterPlayerController.*  Steuerung, Garage/Tuning-Menü, Geld, Spielstand
    ScooterHUD.*           Komplettes HUD inkl. Minimap und Menü
    ScooterWorld.*         Erzeugt Stadt, Parks, Skatepark, Highway, Garage, Straßennetz
    PoliceCar.* / PoliceManager.*   Polizei
    RaceManager.* / RacerBot.*      Rennen, Challenges, Radar
    MotorSynthComponent.*  Prozeduraler Motorsound und Martinshorn
  Tools/SimTest/           Tests für den Simulationskern (ohne Unreal)
```

- Fahrzeug- und Teiledaten stehen als Tabellen in `ScooterSimCore.cpp`. Dort kannst du neue Roller und Teile ergänzen.
- Die Tests laufen unter Linux/macOS mit `Tools/SimTest/build.sh`. Sie prüfen u. a., dass Serienfahrzeuge nicht abbrennen, falsche Settings Kabel durchbrennen lassen, Überspannung den Controller zerstört und Wheelies/Sprünge funktionieren.
- Der Spielstand liegt unter `ScooterSim/Saved/SaveGames/`. Zum Neustart die Datei löschen.

**Hinweis:** Alle Fahrzeug- und Teilewerte sind ungefähre, fürs Gameplay angepasste Werte, keine offiziellen Herstellerangaben. Die Markennamen dienen nur der Wiedererkennung in diesem privaten Hobbyprojekt. Wer das Spiel veröffentlichen will, sollte sie durch eigene Namen ersetzen.
