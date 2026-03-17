#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>

#include "LifeGame.hpp"

class Window
{
public:
	explicit Window(LifeGame& game)
		: m_game(game)
	{
	}

	void Visualize(unsigned numThreads) const
	{
		const unsigned width = m_game.Width();
		const unsigned height = m_game.Height();

		const sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();

		const float maxWindowWidth = static_cast<float>(desktopMode.size.x) * 0.85f;
		const float maxWindowHeight = static_cast<float>(desktopMode.size.y) * 0.85f;

		float scaleX = maxWindowWidth / static_cast<float>(width);
		float scaleY = maxWindowHeight / static_cast<float>(height);

		float scale = std::min(scaleX, scaleY);
		if (scale > 30.0f)
		{
			scale = 30.0f;
		}

		const unsigned maxTexSize = sf::Texture::getMaximumSize();
		if (width > maxTexSize || height > maxTexSize)
		{
			throw std::runtime_error("Grid size exceeds maximum GPU texture size limits (" + std::to_string(maxTexSize) + ")");
		}

		sf::RenderWindow window(
			sf::VideoMode({ static_cast<unsigned>(static_cast<float>(width) * scale),
				static_cast<unsigned>(static_cast<float>(height) * scale) }),
			"Game of Life");
		window.setFramerateLimit(60);

		sf::Image image({ width, height }, sf::Color::White);
		sf::Texture texture;

		if (!texture.loadFromImage(image))
		{
			throw std::runtime_error("Failed to initialize texture");
		}

		sf::Sprite sprite(texture);
		sprite.setScale({ scale, scale });

		double totalTime = 0.0;
		int framesCount = 0;

		sf::Clock fpsClock;
		sf::Clock titleUpdateClock;

		while (window.isOpen())
		{
			while (const auto event = window.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					window.close();
				}
			}

			auto start = std::chrono::high_resolution_clock::now();
			m_game.Update(numThreads);
			auto end = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double> diff = end - start;
			totalTime += diff.count();
			framesCount++;

			if (titleUpdateClock.getElapsedTime().asSeconds() >= 1.0f)
			{
				double avgTime = totalTime / framesCount;
				float fps = static_cast<float>(framesCount) / fpsClock.getElapsedTime().asSeconds();
				window.setTitle(std::format(
					"Game of Life | FPS: {:.2} | Avg calculation time: {:.4} s",
					fps,
					avgTime));
				totalTime = 0.0;
				framesCount = 0;
				titleUpdateClock.restart();
				fpsClock.restart();
			}

			const auto& cells = m_game.Cells();

			for (unsigned y = 0; y < height; ++y)
			{
				for (unsigned x = 0; x < width; ++x)
				{
					bool isAlive = cells[y * width + x] == LifeGame::CellState::Alive;
					image.setPixel({ x, y }, isAlive ? sf::Color::Red : sf::Color::White);
				}
			}

			texture.update(image);

			window.clear(sf::Color::White);
			window.draw(sprite);
			window.display();
		}
	}

private:
	LifeGame& m_game;
};