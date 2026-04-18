#pragma once

#include <bit>
#include <chrono>
#include <cstdint>
#include <iostream>

namespace tv_app
{

#define TV_APP_LOG(msg) std::cout << "[TV App] " << msg << std::endl

enum class PacketType : uint8_t
{
	AudioData,
	VideoData,
	Subscribe
};

constexpr std::size_t MAX_PAYLOAD_SIZE = 2048;
constexpr int MAX_VIDEO_QUEUE_SIZE = 1000;
constexpr int DEFAULT_VIDEO_QUALITY = 95;
constexpr int MIN_VIDEO_QUALITY = 10;

#pragma pack(push, 1)
struct PacketHeader
{
	PacketType type;
	std::uint64_t timestamp;
	std::uint32_t frameId;
	std::uint16_t chunkIndex;
	std::uint16_t totalChunks;
	std::uint16_t payloadSize;
};
#pragma pack(pop)

template <typename DataType>
DataType HostToNetwork(DataType value)
{
	if constexpr (std::endian::native == std::endian::little)
	{
		return std::byteswap(value);
	}

	return value;
}

template <typename DataType>
DataType NetworkToHost(DataType value)
{
	if constexpr (std::endian::native == std::endian::little)
	{
		return std::byteswap(value);
	}

	return value;
}

inline std::uint64_t GetCurrentTimestamp()
{
	static const auto appStartTime = std::chrono::steady_clock::now();
	const auto now = std::chrono::steady_clock::now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(now - appStartTime).count();
}

} // namespace tv_app