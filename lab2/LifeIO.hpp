#pragma once

#include <fstream>
#include <random>
#include <string>

#include "LifeGame.hpp"

namespace LifeIO
{

constexpr char ALIVE_CELL_CHAR = '#';
constexpr char DEAD_CELL_CHAR = ' ';

static LifeGame Load(const std::string& fileName)
{
	std::ifstream in(fileName);
	if (!in.is_open())
	{
		throw std::runtime_error("Cannot open input file: " + fileName);
	}

	unsigned width, height;
	if (!(in >> width >> height))
	{
		throw std::runtime_error("Invalid file format: missing dimensions");
	}

	std::string line;
	std::getline(in, line);

	LifeGame engine(width, height);

	for (unsigned y = 0; y < height; ++y)
	{
		if (!std::getline(in, line))
		{
			break;
		}
		for (unsigned x = 0; x < width && x < line.length(); ++x)
		{
			if (line[x] == ALIVE_CELL_CHAR)
			{
				engine.SetCell(x, y, LifeGame::CellState::Alive);
			}
		}
	}
	return engine;
}

static void Save(const std::string& fileName, const LifeGame& game)
{
	std::ofstream out(fileName);
	if (!out.is_open())
	{
		throw std::runtime_error("Cannot open output file: " + fileName);
	}

	const unsigned w = game.Width();
	const unsigned h = game.Height();
	const auto& cells = game.Cells();

	out << w << " " << h << "\n";
	for (unsigned y = 0; y < h; ++y)
	{
		for (unsigned x = 0; x < w; ++x)
		{
			out << (cells[y * w + x] == LifeGame::CellState::Alive
					? ALIVE_CELL_CHAR
					: DEAD_CELL_CHAR);
		}
		out << "\n";
	}
}

static void Generate(const std::string& outputFileName, int width, int height, double probability)
{
	std::ofstream out(outputFileName);
	if (!out.is_open())
	{
		throw std::runtime_error("Cannot open output file for generation");
	}

	out << width << " " << height << "\n";

	std::random_device rd;
	std::mt19937 gen(rd());
	std::bernoulli_distribution dist(probability);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			out << (dist(gen) ? ALIVE_CELL_CHAR : DEAD_CELL_CHAR);
		}
		out << "\n";
	}
}

} // namespace LifeIO