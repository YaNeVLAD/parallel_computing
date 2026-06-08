#pragma once

#include <PhysicsEngine.hpp>
#include <RenderSystem.hpp>

namespace pc::simulation
{

class Application
{
public:
	Application()
		: m_render(1920, 1080, "[SFML + OpenCL] Particle Simulation")
	{
	}

	void Run()
	{
		while (m_render.IsOpen())
		{
			m_render.ProcessEvents(m_physics);
			m_physics.Update();
			m_render.Render(m_physics);
		}
	}

private:
	PhysicsEngine m_physics;
	RenderSystem m_render;
};

} // namespace pc::simulation