// Prozeduraler Sound: E-Motor-Surren (abhaengig von Drehzahl und Strom), Fahrtwind
// und das Martinshorn der Polizei. Braucht keine Sound-Assets.
#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include <atomic>
#include "MotorSynthComponent.generated.h"

UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class SCOOTERSIM_API UMotorSynthComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UMotorSynthComponent(const FObjectInitializer& ObjectInitializer);

	// Motor: Grundfrequenz in Hz, Lautstaerke 0..1, Wind/Rauschen 0..1
	void SetMotor(float FrequencyHz, float Amplitude, float Noise);
	// Martinshorn an/aus mit Lautstaerke 0..1
	void SetSiren(float Volume);

protected:
	virtual bool Init(int32& InOutSampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<float> TargetFreq { 0.f };
	std::atomic<float> TargetAmp { 0.f };
	std::atomic<float> TargetNoise { 0.f };
	std::atomic<float> TargetSiren { 0.f };

	// Nur im Audio-Thread benutzt
	float SampleRateHz = 48000.f;
	float Phase = 0.f;
	float SirenPhase = 0.f;
	float SirenClock = 0.f;
	float CurFreq = 0.f;
	float CurAmp = 0.f;
	float CurNoise = 0.f;
	float CurSiren = 0.f;
	float NoiseLp = 0.f;
	uint32 NoiseSeed = 22222u;
};
