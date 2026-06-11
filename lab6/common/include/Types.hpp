#pragma once

#include <cstdint>
#include <vector>

inline constexpr std::size_t BUFFER_DEFAULT_SIZE = 256;

struct Pixel
{
	std::uint8_t r;
	std::uint8_t g;
	std::uint8_t b;
};

struct Image
{
	std::vector<Pixel> pixels;
	std::size_t width{};
	std::size_t height{};
};

template <std::size_t Size = BUFFER_DEFAULT_SIZE>
struct HistogramResult
{
	float histR[Size] = { 0.f };
	float histG[Size] = { 0.f };
	float histB[Size] = { 0.f };
};

template <std::size_t Size = BUFFER_DEFAULT_SIZE>
struct HistogramBuffer
{
	std::uint32_t countsR[Size] = {};
	std::uint32_t countsG[Size] = {};
	std::uint32_t countsB[Size] = {};

	HistogramBuffer& operator+=(const HistogramBuffer& other)
	{
		for (std::size_t i = 0; i < Size; ++i)
		{
			countsR[i] += other.countsR[i];
			countsG[i] += other.countsG[i];
			countsB[i] += other.countsB[i];
		}

		return *this;
	}

	friend HistogramBuffer operator+(HistogramBuffer lhs, const HistogramBuffer& rhs)
	{
		lhs += rhs;

		return lhs;
	}
};