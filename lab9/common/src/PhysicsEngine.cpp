#include <PhysicsEngine.hpp>

#include <algorithm>
#include <iostream>
#include <numbers>
#include <random>

namespace
{

constexpr auto s_programName = "ParticlesPhysics";

constexpr auto s_kernelSource = R"(
__kernel void ParticlesPhysics(
	__global const float4* g_posMassIn,
    __global const float2* g_velocitiesIn,
    __global float4* g_posMassOut,
    __global float2* g_velocitiesOut,
    const float G,
    const float dt,
    const float softening,
    const int particlesCount)
{
    int i = get_global_id(0);
    if (i >= particlesCount)
	{
		return;
	}

    float4 posMass = g_posMassIn[i];
    float2 velocityVec = g_velocitiesIn[i];
    float2 accelerationVec = (float2)(0.0f, 0.0f);

    for (int j = 0; j < particlesCount; j++) {
		if (i == j) continue;

        float4 otherPosMass = g_posMassIn[j];

        float2 rangeVec = otherPosMass.xy - posMass.xy;

		// Add softening to avoid division by 0 if particles overlap
		float distSqr = rangeVec.x * rangeVec.x + rangeVec.y * rangeVec.y;
		distSqr = max(distSqr, softening);

        float invDist = rsqrt(distSqr);

		// Scalar formula: acceleration = G * m2 / r^2
		// Scalar -> Vector = Scalar * (rangeVec / distSqr)
		// Vector formula: accelerationVec = G * m2 * (1 / r^3) * rangeVec
        float invDist3 = invDist * invDist * invDist;

		// accelerationVec = G * m2 * (1 / r^3) * rangeVec
        accelerationVec += G * otherPosMass.w * invDist3 * rangeVec;
    }

    velocityVec += accelerationVec * dt;
    posMass.xy += velocityVec * dt;

    g_velocitiesOut[i] = velocityVec;
    g_posMassOut[i] = posMass;
}
)";

} // namespace

namespace pc::simulation
{

PhysicsEngine::PhysicsEngine(PhysicsConfig&& m_config)
	: m_config(m_config)
{
	InitOpenCL();
	InitParticles();
}

void PhysicsEngine::Update()
{
	const unsigned short writeIndex = 1 - readIndex;

	m_kernel.setArg(0, m_clPosMass[readIndex]);
	m_kernel.setArg(1, m_clVelocity[readIndex]);
	m_kernel.setArg(2, m_clPosMass[writeIndex]);
	m_kernel.setArg(3, m_clVelocity[writeIndex]);
	m_kernel.setArg(4, m_config.G);
	m_kernel.setArg(5, m_config.dt);
	m_kernel.setArg(6, m_config.softening);
	m_kernel.setArg(7, static_cast<int>(m_config.currentParticles));

	m_queue.enqueueNDRangeKernel(m_kernel, cl::NullRange, cl::NDRange(m_config.currentParticles), cl::NullRange);

	m_queue.enqueueReadBuffer(m_clPosMass[writeIndex], CL_TRUE, 0, m_config.currentParticles * sizeof(cl_float4), m_posMass.data());
	m_queue.enqueueReadBuffer(m_clVelocity[writeIndex], CL_TRUE, 0, m_config.currentParticles * sizeof(cl_float2), m_velocities.data());

	readIndex = writeIndex;
}

void PhysicsEngine::SpawnParticles(const float x, const float y, const std::size_t count)
{
	if (m_config.currentParticles + count >= m_config.maxParticles)
	{
		return;
	}

	std::vector<cl_float4> newPos(count);
	std::vector<cl_float2> newVel(count);

	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution distRadius(0.0f, 400.0f);

	for (size_t i = 0; i < count; ++i)
	{
		newPos[i] = { x + distRadius(rng) * 0.1f, y + distRadius(rng) * 0.1f, 0.0f, 1.0f };
		newVel[i] = { 0.0f, 0.0f };
	}

	m_queue.enqueueWriteBuffer(m_clPosMass[readIndex], CL_FALSE, m_config.currentParticles * sizeof(cl_float4), count * sizeof(cl_float4), newPos.data());
	m_queue.enqueueWriteBuffer(m_clVelocity[readIndex], CL_FALSE, m_config.currentParticles * sizeof(cl_float2), count * sizeof(cl_float2), newVel.data());

	m_config.currentParticles += count;
}

const std::vector<cl_float4>& PhysicsEngine::Positions() const
{
	return m_posMass;
}

const std::vector<cl_float2>& PhysicsEngine::Velocities() const
{
	return m_velocities;
}

void PhysicsEngine::InitOpenCL()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.empty())
	{
		throw std::runtime_error("No OpenCL platforms found");
	}

	const cl::Platform platform = platforms.front();
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	if (devices.empty())
	{
		throw std::runtime_error("OpenCL GPU device not found");
	}

	cl::Device device = devices.front();
	m_context = cl::Context(device);
	m_queue = cl::CommandQueue(m_context, device);
	m_program = cl::Program(m_context, s_kernelSource);

	try
	{
		m_program.build({ device });
	}
	catch (...)
	{
		std::cerr << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << '\n';
		throw;
	}

	m_kernel = cl::Kernel(m_program, s_programName);
}

void PhysicsEngine::InitParticles()
{
	m_posMass.resize(m_config.maxParticles, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_velocities.resize(m_config.maxParticles, { 0.0f, 0.0f });

	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution distRadius(0.0f, 40.0f);
	std::uniform_real_distribution distAngle(0.0f, 2.f * std::numbers::pi_v<float>);
	std::uniform_real_distribution distMass(1.f, 50.f);

	constexpr float centerX = 1920.0f / 2.0f;
	constexpr float centerY = 1080.0f / 2.0f;

	for (std::size_t i = 0; i < m_config.currentParticles; ++i)
	{
		auto& posMass = m_posMass[i];
		auto& velocity = m_velocities[i];

		const float rangeVec = distRadius(rng);
		const float angle = distAngle(rng);

		posMass.s[0] = centerX + rangeVec * std::cos(angle);
		posMass.s[1] = centerY + rangeVec * std::sin(angle);
		posMass.s[3] = distMass(rng);

		velocity.s[0] = 0;
		velocity.s[1] = 0;
	}

	m_clPosMass[0] = cl::Buffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * m_config.maxParticles, m_posMass.data());
	m_clVelocity[0] = cl::Buffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * m_config.maxParticles, m_velocities.data());

	m_clPosMass[1] = cl::Buffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * m_config.maxParticles, m_posMass.data());
	m_clVelocity[1] = cl::Buffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * m_config.maxParticles, m_velocities.data());
}

} // namespace pc::simulation