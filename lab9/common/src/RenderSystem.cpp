#include <RenderSystem.hpp>

namespace pc
{

RenderSystem::RenderSystem(unsigned int width, unsigned int height, const std::string& title)
	: m_window(sf::VideoMode({ width, height }), title)
	, m_va(sf::PrimitiveType::Points)
{
	m_window.setFramerateLimit(60);
}

bool RenderSystem::IsOpen() const
{
	return m_window.isOpen();
}

void RenderSystem::ProcessEvents(simulation::PhysicsEngine& physics)
{
	auto& config = physics.Config();

	while (const std::optional event = m_window.pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
		}
		else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
		{
			if (keyPressed->scancode == sf::Keyboard::Scancode::Up)
			{
				config.G += 10.0f;
			}
			if (keyPressed->scancode == sf::Keyboard::Scancode::Down)
			{
				config.G = std::max(0.0f, config.G - 10.0f);
			}
			if (keyPressed->scancode == sf::Keyboard::Scancode::Right)
			{
				config.dt += 0.001f;
			}
			if (keyPressed->scancode == sf::Keyboard::Scancode::Left)
			{
				config.dt = std::max(0.0f, config.dt - 0.001f);
			}
		}
		else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if (mousePressed->button == sf::Mouse::Button::Left)
			{
				physics.SpawnParticles(
					static_cast<float>(mousePressed->position.x),
					static_cast<float>(mousePressed->position.y));
			}
		}
	}
}

void RenderSystem::Render(const simulation::PhysicsEngine& physics)
{
	const auto& config = physics.Config();

	const std::size_t count = config.currentParticles;
	if (m_va.getVertexCount() != count)
	{
		m_va.resize(count);
	}

	const auto& pos = physics.Positions();
	const auto& vel = physics.Velocities();

	m_window.clear(sf::Color(10, 10, 15));

	for (std::size_t i = 0; i < count; ++i)
	{
		const auto& p = pos[i];
		const auto& v = vel[i];

		m_va[i].position = { p.s[0], p.s[1] };

		const float speed = std::sqrt(v.s[0] * v.s[0] + v.s[1] * v.s[1]);
		const auto colorIntensity = static_cast<std::uint8_t>(std::min(255.0f, speed * 5.0f));
		const auto invColorIntensity = static_cast<std::uint8_t>(255 - colorIntensity);

		m_va[i].color = sf::Color(colorIntensity, 100, invColorIntensity);
		// m_va[i].color = sf::Color::White;
	}

	m_window.draw(m_va);

	const std::string title = "Particles: "
		+ std::to_string(count)
		+ " | G: " + std::to_string(config.G)
		+ " | dt: " + std::to_string(config.dt);

	m_window.setTitle(title);
	m_window.display();
}

} // namespace pc