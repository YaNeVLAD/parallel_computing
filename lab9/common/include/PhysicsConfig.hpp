#pragma once

#include <cstddef>

namespace pc::simulation
{

struct PhysicsConfig
{
	std::size_t maxParticles = 150000;
	std::size_t currentParticles = 30000;

	float G = 0.01f;
	float dt = 0.02f;
	float softening = 0.01f;
};

} // namespace pc::simulation