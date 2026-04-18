#pragma once

#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>

#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include <rtaudio/RtAudio.h>

#include <Common.hpp>

namespace tv_app
{

using boost::asio::ip::udp;

class Receiver
{
public:
	Receiver(boost::asio::io_context& ioContext, const std::string& address, const int port)
		: m_running(true)
		, m_socket(ioContext, udp::endpoint(udp::v4(), 0))
		, m_stationEndpoint(boost::asio::ip::make_address(address), port)
		, m_timer(ioContext)
	{
		Subscribe();
		StartReceive();
		StartAudioOutput();
	}

	~Receiver()
	{
		m_running = false;
		try
		{
			if (m_dac.isStreamOpen())
			{
				m_dac.stopStream();
				m_dac.closeStream();
			}
		}
		catch (...)
		{
		}
	}

	void RunDisplayLoop()
	{
		while (m_running)
		{
			cv::Mat frameToDisplay;
			{
				std::lock_guard lock(m_videoMutex);
				if (!m_videoFrames.empty())
				{
					if (const auto it = m_videoFrames.begin();
						it->first <= m_currentAudioTimestamp || m_currentAudioTimestamp == 0)
					{
						frameToDisplay = it->second;
						m_videoFrames.erase(it);
					}
				}
			}

			if (!frameToDisplay.empty())
			{
				cv::imshow("TV Receiver", frameToDisplay);
				cv::waitKey(1);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
		}
	}

	void Stop()
	{
		m_running = false;
	}

private:
	struct FrameAssembly
	{
		std::map<std::uint16_t, std::vector<std::uint8_t>> chunks;
		std::uint16_t receivedChunks = 0;
	};

	void Subscribe()
	{
		PacketHeader header{};
		header.type = PacketType::Subscribe;

		m_socket.send_to(boost::asio::buffer(&header, sizeof(header)), m_stationEndpoint);

		m_timer.expires_after(std::chrono::seconds(5));
		m_timer.async_wait([this](const boost::system::error_code& ec) {
			if (!ec && m_running)
			{
				Subscribe();
			}
		});
	}

	void StartAudioOutput()
	{
		if (m_dac.getDeviceCount() < 1)
		{
			TV_APP_LOG("No audio output devices found!");
			return;
		}

		RtAudio::StreamParameters parameters;
		parameters.deviceId = m_dac.getDefaultOutputDevice();
		parameters.nChannels = 1;
		parameters.firstChannel = 0;

		unsigned int bufferFrames = 480;

		try
		{
			constexpr unsigned int SAMPLE_RATE = 48000;

			m_dac.openStream(&parameters, nullptr, RTAUDIO_SINT16,
				SAMPLE_RATE, &bufferFrames, &Receiver::AudioOutputCallback, this);
			m_dac.startStream();

			TV_APP_LOG("Audio playback started.");
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}

	void StartReceive()
	{
		m_socket.async_receive_from(
			boost::asio::buffer(m_responseBuffer), m_remoteEndpoint,
			[this](const boost::system::error_code& ec, const std::size_t bytes) {
				if (!ec && bytes >= sizeof(PacketHeader))
				{
					ProcessPacket(bytes);
				}
				if (m_running)
				{
					StartReceive();
				}
			});
	}

	void ProcessPacket(const std::size_t responseBytes)
	{
		PacketHeader header{};
		std::memcpy(&header, m_responseBuffer.data(), sizeof(PacketHeader));

		header.timestamp = NetworkToHost(header.timestamp);
		header.frameId = NetworkToHost(header.frameId);
		header.chunkIndex = NetworkToHost(header.chunkIndex);
		header.totalChunks = NetworkToHost(header.totalChunks);
		header.payloadSize = NetworkToHost(header.payloadSize);

		constexpr std::size_t PAYLOAD_OFFSET = sizeof(PacketHeader);
		if (responseBytes < PAYLOAD_OFFSET + header.payloadSize)
		{
			return;
		}

		if (header.type == PacketType::AudioData)
		{
			if (header.timestamp < m_currentAudioTimestamp)
			{
				return;
			}

			m_currentAudioTimestamp = header.timestamp;

			std::vector<std::uint8_t> audioPayload(
				m_responseBuffer.data() + PAYLOAD_OFFSET,
				m_responseBuffer.data() + PAYLOAD_OFFSET + header.payloadSize);

			std::lock_guard lock(m_audioMutex);
			m_audioQueue.emplace(std::move(audioPayload));
		}
		else if (header.type == PacketType::VideoData)
		{
			ReassembleVideo(header, m_responseBuffer.data() + PAYLOAD_OFFSET);
		}
	}

	void ReassembleVideo(const PacketHeader& header, const char* payload)
	{
		std::lock_guard lock(m_videoMutex);

		constexpr std::uint32_t OLD_FRAME_ID_LIMIT = 20;
		std::erase_if(m_frameAssembly, [header](const auto& item) {
			return item.first < (header.frameId > OLD_FRAME_ID_LIMIT ? header.frameId - OLD_FRAME_ID_LIMIT : 0);
		});

		auto& [chunks, receivedChunks] = m_frameAssembly[header.frameId];
		chunks[header.chunkIndex] = std::vector<std::uint8_t>(payload, payload + header.payloadSize);
		receivedChunks++;

		if (receivedChunks == header.totalChunks)
		{
			std::vector<std::uint8_t> completeBuffer;
			completeBuffer.reserve(header.totalChunks * MAX_PAYLOAD_SIZE);

			for (std::uint16_t i = 0; i < header.totalChunks; ++i)
			{
				completeBuffer.insert(completeBuffer.end(), chunks[i].begin(), chunks[i].end());
			}

			if (const cv::Mat decodedImage = cv::imdecode(completeBuffer, cv::IMREAD_COLOR); !decodedImage.empty())
			{
				m_videoFrames[header.timestamp] = decodedImage;
			}

			m_frameAssembly.erase(header.frameId);
		}
	}

	static int AudioOutputCallback(void* outputBuffer, void* /*inputBuffer*/, const unsigned int nFrames,
		double /*streamTime*/, RtAudioStreamStatus /*status*/, void* userData)
	{
		auto* receiver = static_cast<Receiver*>(userData);
		const std::size_t bytesNeeded = nFrames * sizeof(std::int16_t);
		auto* outBuffer = static_cast<uint8_t*>(outputBuffer);

		std::lock_guard lock(receiver->m_audioMutex);

		if (!receiver->m_audioQueue.empty())
		{
			const auto& chunk = receiver->m_audioQueue.front();
			const std::size_t copySize = std::min(bytesNeeded, chunk.size());

			std::memcpy(outBuffer, chunk.data(), copySize);
			receiver->m_audioQueue.pop();

			if (copySize < bytesNeeded)
			{
				std::memset(outBuffer + copySize, 0, bytesNeeded - copySize);
			}
		}
		else
		{
			std::memset(outBuffer, 0, bytesNeeded);
		}

		return 0;
	}

	std::atomic_bool m_running;

	udp::socket m_socket;
	udp::endpoint m_stationEndpoint;
	udp::endpoint m_remoteEndpoint;
	boost::asio::steady_timer m_timer;
	std::array<char, 65536> m_responseBuffer{};

	std::mutex m_videoMutex;
	std::map<std::uint32_t, FrameAssembly> m_frameAssembly;
	std::map<std::uint64_t, cv::Mat> m_videoFrames;

	std::atomic<std::uint64_t> m_currentAudioTimestamp{ 0 };

	RtAudio m_dac;
	std::mutex m_audioMutex;
	std::queue<std::vector<std::uint8_t>> m_audioQueue;
};

} // namespace tv_app