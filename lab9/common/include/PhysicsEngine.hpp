#pragma once

#include <OpenCL.hpp>
#include <PhysicsConfig.hpp>

namespace pc::simulation
{

class PhysicsEngine
{
public:
	explicit PhysicsEngine(PhysicsConfig&& m_config = {});

	void Update();

	void SpawnParticles(float x, float y, std::size_t count = 100);

	[[nodiscard]] const std::vector<cl_float4>& Positions() const;

	[[nodiscard]] const std::vector<cl_float2>& Velocities() const;

	template <typename TSelf>
	[[nodiscard]] auto&& Config(this TSelf&& self)
	{
		return std::forward<TSelf>(self).m_config;
	}

private:
	void InitOpenCL();

	void InitParticles();

private:
	cl::Context m_context;
	cl::CommandQueue m_queue;
	cl::Program m_program;
	cl::Kernel m_kernel;

	// { x, y, 0, mass }[]
	std::vector<cl_float4> m_posMass;

	// { vx, vy }[]
	std::vector<cl_float2> m_velocities;

	cl::Buffer m_clPosMass[2];
	cl::Buffer m_clVelocity[2];

	unsigned short readIndex = 0;

	PhysicsConfig m_config;
};

} // namespace pc::simulation
