#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include <boost/asio.hpp>

#include <Common.hpp>

namespace tv_app
{

using boost::asio::ip::udp;

class Station
{
public:
	Station(boost::asio::io_context& ioContext, const int port)
		: m_socket(ioContext, udp::endpoint(udp::v4(), port))
		, m_jpegQuality(DEFAULT_VIDEO_QUALITY)
	{
		StartReceive();
	}

	void BroadcastVideo(const std::vector<std::uint8_t>& data, const std::uint64_t timestamp, const std::uint32_t frameId)
	{
		ChunkAndQueueData(PacketType::VideoData, data, timestamp, frameId);
	}

	void BroadcastAudio(const std::vector<std::uint8_t>& data, const std::uint64_t timestamp)
	{
		ChunkAndQueueData(PacketType::AudioData, data, timestamp, 0);
	}

	int CalculateRecommendedQuality()
	{
		std::lock_guard lock(m_queueMutex);
		if (m_videoQueueSize > MAX_VIDEO_QUEUE_SIZE)
		{
			m_jpegQuality = std::max(MIN_VIDEO_QUALITY, m_jpegQuality - 10);
			TV_APP_LOG("Network overload. Degraded video quality to " << m_jpegQuality);
		}
		else if (m_videoQueueSize < (MAX_VIDEO_QUEUE_SIZE / 4) && m_jpegQuality < DEFAULT_VIDEO_QUALITY)
		{
			m_jpegQuality = std::min(DEFAULT_VIDEO_QUALITY, m_jpegQuality + 2);
		}

		return m_jpegQuality;
	}

private:
	void StartReceive()
	{
		m_socket.async_receive_from(
			boost::asio::buffer(m_subReqBuffer), m_remoteEndpoint,
			[this](const boost::system::error_code& ec, const std::size_t bytesRecv) {
				if (!ec && bytesRecv == sizeof(PacketHeader))
				{
					PacketHeader header{};
					std::memcpy(&header, m_subReqBuffer.data(), sizeof(PacketHeader));
					if (header.type == PacketType::Subscribe)
					{
						std::lock_guard lock(m_clientsMutex);
						if (std::ranges::find(m_clients, m_remoteEndpoint) == m_clients.end())
						{
							TV_APP_LOG("New receiver connected: " << m_remoteEndpoint.address().to_string());
							m_clients.push_back(m_remoteEndpoint);
						}
					}
				}
				StartReceive();
			});
	}

	void ChunkAndQueueData(PacketType type, const std::vector<std::uint8_t>& data, const std::uint64_t timestamp, const std::uint32_t frameId)
	{
		const auto totalChunks = static_cast<std::uint16_t>((data.size() + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE);

		std::vector<udp::endpoint> localClients;
		{
			std::lock_guard lock(m_clientsMutex);
			if (m_clients.empty())
			{
				return;
			}
			localClients = m_clients;
		}

		for (std::uint16_t i = 0; i < totalChunks; ++i)
		{
			const std::size_t offset = i * MAX_PAYLOAD_SIZE;
			const std::size_t chunkSize = std::min(MAX_PAYLOAD_SIZE, data.size() - offset);

			PacketHeader header{};
			header.type = type;
			header.timestamp = HostToNetwork(timestamp);
			header.frameId = HostToNetwork(frameId);
			header.chunkIndex = HostToNetwork(i);
			header.totalChunks = HostToNetwork(totalChunks);
			header.payloadSize = HostToNetwork(static_cast<std::uint16_t>(chunkSize));

			auto packet = std::make_shared<std::vector<std::uint8_t>>(sizeof(PacketHeader) + chunkSize);

			std::memcpy(packet->data(), &header, sizeof(PacketHeader));
			std::memcpy(packet->data() + sizeof(PacketHeader), data.data() + offset, chunkSize);

			for (const auto& client : localClients)
			{
				auto endpointPtr = std::make_shared<udp::endpoint>(client);

				if (type == PacketType::VideoData)
				{
					std::lock_guard qLock(m_queueMutex);
					m_videoQueueSize++;
				}

				m_socket.async_send_to(
					boost::asio::buffer(*packet), *endpointPtr,
					[this, type, packet, endpointPtr](boost::system::error_code, std::size_t) {
						if (type == PacketType::VideoData)
						{
							std::lock_guard qLock(m_queueMutex);
							m_videoQueueSize--;
						}
					});
			}
		}
	}

	udp::socket m_socket;
	udp::endpoint m_remoteEndpoint;
	std::array<char, 1024> m_subReqBuffer{};

	std::vector<udp::endpoint> m_clients;
	std::mutex m_clientsMutex;

	std::mutex m_queueMutex;
	int m_videoQueueSize = 0;
	int m_jpegQuality;
};

} // namespace tv_app