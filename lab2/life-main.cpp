#include <SFML/Graphics.hpp>

#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>

#include "LifeGame.hpp"
#include "LifeIO.hpp"
#include "Window.hpp"

void PrintHelp()
{
	std::cout << "Usage:\n"
			  << "  life generate OUTPUT_FILE_NAME WIDTH HEIGHT PROBABILITY\n"
			  << "  life step INPUT_FILE_NAME NUM_THREADS [OUTPUT_FILE_NAME]\n"
			  << "  life visualize INPUT_FILE_NAME NUM_THREADS\n";
}

int main(const int argc, char* argv[])
{
	if (argc < 2)
	{
		PrintHelp();
		return 1;
	}

	const std::string command = argv[1];

	try
	{
		if (command == "generate" && argc == 6)
		{
			const std::string outputFileName = argv[2];
			const int width = std::stoi(argv[3]);
			const int height = std::stoi(argv[4]);
			if (width <= 0 || height <= 0)
			{
				throw std::runtime_error("Width and height must be positive");
			}

			const double probability = std::stod(argv[5]);

			LifeIO::Generate(outputFileName, width, height, probability);
			std::cout << "Grid generated successfully.\n";
		}
		else if (command == "step" && (argc == 4 || argc == 5))
		{
			const std::string inputFileName = argv[2];
			const int numThreads = std::stoi(argv[3]);
			if (numThreads <= 0)
			{
				throw std::runtime_error("Number of threads must be positive");
			}

			const std::string outputFileName = (argc == 5) ? argv[4] : "";

			auto game = LifeIO::Load(inputFileName);

			const auto start = std::chrono::high_resolution_clock::now();
			game.Update(numThreads);
			const auto end = std::chrono::high_resolution_clock::now();

			const std::chrono::duration<double> diff = end - start;
			std::cout << "Step calculation time: " << diff.count() << " seconds\n";

			LifeIO::Save(outputFileName.empty() ? inputFileName : outputFileName, game);
		}
		else if (command == "visualize" && argc == 4)
		{
			const std::string inputFileName = argv[2];
			const int numThreads = std::stoi(argv[3]);
			if (numThreads <= 0)
			{
				throw std::runtime_error("Number of threads must be positive");
			}

			auto game = LifeIO::Load(inputFileName);
			Window window(game);
			window.Visualize(numThreads);
		}
		else
		{
			PrintHelp();
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}