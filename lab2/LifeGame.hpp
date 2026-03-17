#pragma once

#include <thread>
#include <vector>

class LifeGame
{
public:
	enum class CellState : uint8_t
	{
		Dead = 0,
		Alive = 1
	};

	LifeGame(const unsigned width, const unsigned height)
		: m_width(width)
		, m_height(height)
	{
		m_frontBuffer.assign(width * height, CellState::Dead);
		m_backBuffer.assign(width * height, CellState::Dead);
	}

	void Update(const unsigned numThreads)
	{
		std::vector<std::jthread> workers;
		const unsigned rowsPerThread = m_height / numThreads;

		for (int i = 0; i < numThreads; ++i)
		{
			unsigned startY = i * rowsPerThread;
			unsigned endY = (i == numThreads - 1) ? m_height : (i + 1) * rowsPerThread;
			workers.emplace_back(&LifeGame::ComputeRegion, this, startY, endY);
		}
		workers.clear();
		std::swap(m_frontBuffer, m_backBuffer);
	}

	unsigned Width() const
	{
		return m_width;
	}

	unsigned Height() const
	{
		return m_height;
	}

	const std::vector<CellState>& Cells() const
	{
		return m_frontBuffer;
	}

	void SetCell(const unsigned x, const unsigned y, const CellState state)
	{
		m_frontBuffer[y * m_width + x] = state;
	}

private:
	void ComputeRegion(const unsigned startY, const unsigned endY)
	{
		for (unsigned y = startY; y < endY; ++y)
		{
			for (unsigned x = 0; x < m_width; ++x)
			{
				const unsigned aliveNeighbors = CountAliveNeighbors(x, y);
				const CellState current = m_frontBuffer[y * m_width + x];

				if (current == CellState::Alive)
				{
					m_backBuffer[y * m_width + x] = (aliveNeighbors == 2 || aliveNeighbors == 3)
						? CellState::Alive
						: CellState::Dead;
				}
				else
				{
					m_backBuffer[y * m_width + x] = (aliveNeighbors == 3)
						? CellState::Alive
						: CellState::Dead;
				}
			}
		}
	}

	unsigned CountAliveNeighbors(const unsigned x, const unsigned y) const
	{
		unsigned count = 0;
		for (int dy = -1; dy <= 1; ++dy)
		{
			for (int dx = -1; dx <= 1; ++dx)
			{
				if (dx == 0 && dy == 0)
				{
					continue;
				}
				const auto nx = (x + dx + m_width) % m_width;
				const auto ny = (y + dy + m_height) % m_height;
				if (m_frontBuffer[ny * m_width + nx] == CellState::Alive)
				{
					count++;
				}
			}
		}
		return count;
	}

	unsigned m_width{};
	unsigned m_height{};

	std::vector<CellState> m_frontBuffer;
	std::vector<CellState> m_backBuffer;
};