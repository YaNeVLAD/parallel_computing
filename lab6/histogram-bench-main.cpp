// ReSharper disable CppDFAUnreadVariable

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <execution>
#include <numeric>
#include <span>
#include <thread>
#include <vector>

#include <Types.hpp>

Image GenerateTestImage(const std::size_t width, const std::size_t height, const bool solidColor = false)
{
	Image img;
	img.width = width;
	img.height = height;
	img.pixels.resize(width * height);

	if (solidColor)
	{
		std::ranges::fill(img.pixels, Pixel{ 128, 128, 128 });
	}
	else
	{
		std::uint32_t state = 12345;
		for (auto& [r, g, b] : img.pixels)
		{
			state ^= state >> 17;
			state ^= state << 13;
			state ^= state << 5;
			r = static_cast<std::uint8_t>(state % 256);
			g = static_cast<std::uint8_t>((state >> 8) % 256);
			b = static_cast<std::uint8_t>((state >> 16) % 256);
		}
	}
	return img;
}

HistogramResult<> NormalizeHistogram(const HistogramBuffer<>& buffer, const std::size_t totalPixels)
{
	HistogramResult result;
	const float invTotal = 1.0f / static_cast<float>(totalPixels);

	for (std::size_t i = 0; i < 256; ++i)
	{
		result.histR[i] = static_cast<float>(buffer.countsR[i]) * invTotal;
		result.histG[i] = static_cast<float>(buffer.countsG[i]) * invTotal;
		result.histB[i] = static_cast<float>(buffer.countsB[i]) * invTotal;
	}

	return result;
}

HistogramResult<> ComputeHistogramSingle(const Image& img)
{
	HistogramBuffer buffer;
	for (const auto& [r, g, b] : img.pixels)
	{
		buffer.countsR[r]++;
		buffer.countsG[g]++;
		buffer.countsB[b]++;
	}

	return NormalizeHistogram(buffer, img.pixels.size());
}

HistogramResult<> ComputeHistogramAtomicInterleaved(const Image& img, const std::size_t threadCount)
{
	std::atomic<std::uint32_t> atomics[256 * 3];
	for (auto& a : atomics)
	{
		a.store(0, std::memory_order_relaxed);
	}

	auto worker = [&](const std::size_t start, const std::size_t end) {
		for (std::size_t i = start; i < end; ++i)
		{
			const auto& [r, g, b] = img.pixels[i];
			atomics[r * 3 + 0].fetch_add(1, std::memory_order_relaxed);
			atomics[g * 3 + 1].fetch_add(1, std::memory_order_relaxed);
			atomics[b * 3 + 2].fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> threads;

	const std::size_t chunkSize = img.pixels.size() / threadCount;
	for (std::size_t t = 0; t < threadCount; ++t)
	{
		std::size_t start = t * chunkSize;
		std::size_t end = t == threadCount - 1
			? img.pixels.size()
			: start + chunkSize;
		threads.emplace_back(worker, start, end);
	}
	threads.clear();

	HistogramBuffer buffer;
	for (std::size_t i = 0; i < 256; ++i)
	{
		buffer.countsR[i] = atomics[i * 3 + 0].load(std::memory_order_relaxed);
		buffer.countsG[i] = atomics[i * 3 + 1].load(std::memory_order_relaxed);
		buffer.countsB[i] = atomics[i * 3 + 2].load(std::memory_order_relaxed);
	}

	return NormalizeHistogram(buffer, img.pixels.size());
}

HistogramResult<> ComputeHistogramAtomicPlanar(const Image& img, const std::size_t threadCount)
{
	std::atomic<std::uint32_t> atomicR[256];
	std::atomic<std::uint32_t> atomicG[256];
	std::atomic<std::uint32_t> atomicB[256];

	for (std::size_t i = 0; i < 256; ++i)
	{
		atomicR[i].store(0, std::memory_order_relaxed);
		atomicG[i].store(0, std::memory_order_relaxed);
		atomicB[i].store(0, std::memory_order_relaxed);
	}

	auto worker = [&](const std::size_t start, const std::size_t end) {
		for (std::size_t i = start; i < end; ++i)
		{
			const auto& [r, g, b] = img.pixels[i];
			atomicR[r].fetch_add(1, std::memory_order_relaxed);
			atomicG[g].fetch_add(1, std::memory_order_relaxed);
			atomicB[b].fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> threads;
	const std::size_t chunkSize = img.pixels.size() / threadCount;
	for (std::size_t t = 0; t < threadCount; ++t)
	{
		std::size_t start = t * chunkSize;
		std::size_t end = t == threadCount - 1
			? img.pixels.size()
			: start + chunkSize;
		threads.emplace_back(worker, start, end);
	}
	threads.clear();

	HistogramBuffer buffer;
	for (std::size_t i = 0; i < 256; ++i)
	{
		buffer.countsR[i] = atomicR[i].load(std::memory_order_relaxed);
		buffer.countsG[i] = atomicG[i].load(std::memory_order_relaxed);
		buffer.countsB[i] = atomicB[i].load(std::memory_order_relaxed);
	}

	return NormalizeHistogram(buffer, img.pixels.size());
}

HistogramResult<> ComputeHistogramThreadLocal(const Image& img, const std::size_t threadCount)
{
	std::vector<HistogramBuffer<>> localBuffers(threadCount);

	auto worker = [&](const std::size_t start, const std::size_t end, const std::size_t threadIdx) {
		auto& [countsR, countsG, countsB] = localBuffers[threadIdx];
		for (std::size_t i = start; i < end; ++i)
		{
			const auto& [r, g, b] = img.pixels[i];
			countsR[r]++;
			countsG[g]++;
			countsB[b]++;
		}
	};

	std::vector<std::jthread> threads;

	const std::size_t chunkSize = img.pixels.size() / threadCount;
	for (std::size_t threadIdx = 0; threadIdx < threadCount; ++threadIdx)
	{
		std::size_t start = threadIdx * chunkSize;
		std::size_t end = threadIdx == threadCount - 1
			? img.pixels.size()
			: start + chunkSize;
		threads.emplace_back(worker, start, end, threadIdx);
	}
	threads.clear();

	HistogramBuffer finalBuffer;
	for (const auto& lb : localBuffers)
	{
		finalBuffer += lb;
	}

	return NormalizeHistogram(finalBuffer, img.pixels.size());
}

HistogramResult<> ComputeHistogramParallelAlgo(const Image& img, const std::size_t threadCount)
{
	const std::size_t chunkSize = std::max<std::size_t>(1024, img.pixels.size() / threadCount);
	std::vector<std::span<const Pixel>> chunks;

	for (std::size_t i = 0; i < img.pixels.size(); i += chunkSize)
	{
		std::size_t size = std::min(chunkSize, img.pixels.size() - i);
		chunks.emplace_back(img.pixels.data() + i, size);
	}

	const HistogramBuffer finalBuffer = std::transform_reduce(
		std::execution::par_unseq,
		chunks.begin(),
		chunks.end(),
		HistogramBuffer{},
		std::plus{}, [](std::span<const Pixel> chunk) {
			HistogramBuffer localBuffer;
			for (const auto& [r, g, b] : chunk)
			{
				localBuffer.countsR[r]++;
				localBuffer.countsG[g]++;
				localBuffer.countsB[b]++;
			}

			return localBuffer;
		});

	return NormalizeHistogram(finalBuffer, img.pixels.size());
}

Image g_cachedImage;

void SetupImage(const benchmark::State& /*state*/)
{
	if (g_cachedImage.pixels.empty())
	{
		// ~24m pixels, non solid
		g_cachedImage = GenerateTestImage(6000, 4000, false);
	}
}

static void BM_SingleThread(benchmark::State& state)
{
	SetupImage(state);
	for (auto _ : state)
	{
		auto result = ComputeHistogramSingle(g_cachedImage);
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_SingleThread)->Unit(benchmark::kMillisecond);

static void BM_AtomicInterleaved(benchmark::State& state)
{
	SetupImage(state);
	const std::size_t threadCount = state.range(0);
	for (auto _ : state)
	{
		auto result = ComputeHistogramAtomicInterleaved(g_cachedImage, threadCount);
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_AtomicInterleaved)->RangeMultiplier(2)->Range(1, 16)->Unit(benchmark::kMillisecond);

static void BM_AtomicPlanar(benchmark::State& state)
{
	SetupImage(state);
	const std::size_t threadCount = state.range(0);
	for (auto _ : state)
	{
		auto result = ComputeHistogramAtomicPlanar(g_cachedImage, threadCount);
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_AtomicPlanar)->RangeMultiplier(2)->Range(1, 16)->Unit(benchmark::kMillisecond);

static void BM_ThreadLocal(benchmark::State& state)
{
	SetupImage(state);
	const std::size_t threadCount = state.range(0);
	for (auto _ : state)
	{
		auto result = ComputeHistogramThreadLocal(g_cachedImage, threadCount);
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_ThreadLocal)->RangeMultiplier(2)->Range(1, 16)->Unit(benchmark::kMillisecond);

static void BM_ParallelAlgo(benchmark::State& state)
{
	SetupImage(state);
	const std::size_t threadCount = state.range(0);
	for (auto _ : state)
	{
		auto result = ComputeHistogramParallelAlgo(g_cachedImage, threadCount);
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_ParallelAlgo)->RangeMultiplier(2)->Range(1, 16)->Unit(benchmark::kMillisecond);

Image g_cachedSolidImage;
static void BM_AtomicPlanar_Solid(benchmark::State& state)
{
	if (g_cachedSolidImage.pixels.empty())
	{
		// Solid Color
		g_cachedSolidImage = GenerateTestImage(6000, 4000, true);
	}

	const std::size_t threadCount = state.range(0);
	for (auto _ : state)
	{
		auto result = ComputeHistogramAtomicPlanar(g_cachedSolidImage, threadCount);
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_AtomicPlanar_Solid)->RangeMultiplier(2)->Range(1, 16)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();