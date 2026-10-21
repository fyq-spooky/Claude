#include "MotorSynthComponent.h"

UMotorSynthComponent::UMotorSynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UMotorSynthComponent::Init(int32& InOutSampleRate)
{
	NumChannels = 1;
	SampleRateHz = (float)FMath::Max(InOutSampleRate, 8000);
	return true;
}

void UMotorSynthComponent::SetMotor(float FrequencyHz, float Amplitude, float Noise)
{
	TargetFreq.store(FMath::Clamp(FrequencyHz, 0.f, 6000.f));
	TargetAmp.store(FMath::Clamp(Amplitude, 0.f, 1.f));
	TargetNoise.store(FMath::Clamp(Noise, 0.f, 1.f));
}

void UMotorSynthComponent::SetSiren(float Volume)
{
	TargetSiren.store(FMath::Clamp(Volume, 0.f, 1.f));
}

int32 UMotorSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const float TF = TargetFreq.load();
	const float TA = TargetAmp.load();
	const float TN = TargetNoise.load();
	const float TS = TargetSiren.load();
	const float Inv = 1.f / SampleRateHz;
	const float TwoPi = 2.f * UE_PI;

	for (int32 i = 0; i < NumSamples; ++i)
	{
		CurFreq += (TF - CurFreq) * 0.0015f;
		CurAmp += (TA - CurAmp) * 0.002f;
		CurNoise += (TN - CurNoise) * 0.002f;
		CurSiren += (TS - CurSiren) * 0.001f;

		float Sample = 0.f;

		if (CurAmp > 0.0005f && CurFreq > 1.f)
		{
			Phase += CurFreq * Inv;
			Phase -= FMath::FloorToFloat(Phase);
			const float A = TwoPi * Phase;
			// Grundton + Obertoene ergeben das typische Surren eines BLDC-Motors
			const float Tone = 0.55f * FMath::Sin(A) + 0.25f * FMath::Sin(2.f * A) + 0.12f * FMath::Sin(3.f * A) + 0.08f * (2.f * Phase - 1.f);
			Sample += Tone * CurAmp;
		}

		if (CurNoise > 0.0005f)
		{
			NoiseSeed = NoiseSeed * 1664525u + 1013904223u;
			const float White = (float)((NoiseSeed >> 9) & 0x7FFFFF) / 8388607.f * 2.f - 1.f;
			NoiseLp += (White - NoiseLp) * 0.06f;
			Sample += NoiseLp * CurNoise;
		}

		if (CurSiren > 0.0005f)
		{
			// Martinshorn: zwei Toene im Wechsel
			SirenClock += Inv;
			if (SirenClock > 1.4f)
			{
				SirenClock -= 1.4f;
			}
			const float F = SirenClock < 0.7f ? 440.f : 587.f;
			SirenPhase += F * Inv;
			SirenPhase -= FMath::FloorToFloat(SirenPhase);
			const float S = FMath::Sin(TwoPi * SirenPhase);
			Sample += (0.7f * S + 0.3f * (S > 0.f ? 1.f : -1.f)) * 0.35f * CurSiren;
		}

		OutAudio[i] = FMath::Clamp(Sample, -1.f, 1.f);
	}
	return NumSamples;
}
