#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <PhysicsEngine.hpp>

namespace pc
{

class RenderSystem
{
public:
	RenderSystem(unsigned int width, unsigned int height, const std::string& title);

	[[nodiscard]] bool IsOpen() const;

	void ProcessEvents(simulation::PhysicsEngine& physics);

	void Render(const simulation::PhysicsEngine& physics);

private:
	sf::RenderWindow m_window;
	sf::VertexArray m_va;
};

} // namespace pc