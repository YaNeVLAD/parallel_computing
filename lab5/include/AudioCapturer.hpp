#pragma once

#include <functional>
#include <iostream>
#include <vector>

#include <rtaudio/RtAudio.h>

#include <Common.hpp>

namespace tv_app
{

class AudioCapturer
{
public:
	using AudioReadyCallback = std::function<void(const std::vector<std::uint8_t>&, std::uint64_t)>;

	explicit AudioCapturer(AudioReadyCallback onAudio)
		: m_onAudio(std::move(onAudio))
	{
	}

	~AudioCapturer()
	{
		Stop();
	}

	void Start()
	{
		if (m_adc.getDeviceCount() < 1)
		{
			TV_APP_LOG("No audio capture devices found!");
			return;
		}

		RtAudio::StreamParameters parameters;
		parameters.deviceId = m_adc.getDefaultInputDevice();
		parameters.nChannels = 1;
		parameters.firstChannel = 0;

		try
		{
			constexpr unsigned int SAMPLE_RATE = 48000;
			unsigned int bufferFrames = 480;

			m_adc.openStream(nullptr,
				&parameters, RTAUDIO_SINT16, SAMPLE_RATE,
				&bufferFrames, &AudioCapturer::AudioInputCallback, this);
			m_adc.startStream();

			TV_APP_LOG("Audio stream started.");
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

	void Stop()
	{
		try
		{
			if (m_adc.isStreamOpen())
			{
				m_adc.stopStream();
				m_adc.closeStream();
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

private:
	static int AudioInputCallback(void* /*outputBuffer*/, void* inputBuffer, const unsigned int nFrames,
		double /*streamTime*/, const RtAudioStreamStatus status, void* userData)
	{
		if (status)
		{
			TV_APP_LOG("Audio stream over/underflow!");
		}
		if (!inputBuffer)
		{
			return 1;
		}

		const auto* capturer = static_cast<AudioCapturer*>(userData);
		const std::size_t bytesCount = nFrames * sizeof(std::int16_t);
		auto* rawAudio = static_cast<std::uint8_t*>(inputBuffer);

		const std::vector audioData(rawAudio, rawAudio + bytesCount);

		if (capturer->m_onAudio)
		{
			capturer->m_onAudio(audioData, GetCurrentTimestamp());
		}

		return 0;
	}

	RtAudio m_adc;
	AudioReadyCallback m_onAudio;
};

} // namespace tv_app