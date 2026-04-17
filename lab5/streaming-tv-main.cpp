#include "Receiver.hpp"

#include <atomic>
#include <iostream>
#include <string>

#include <AudioCapturer.hpp>
#include <Common.hpp>
#include <Station.hpp>
#include <VideoCapturer.hpp>

int main(const int argc, char* argv[])
{
	try
	{
		std::atomic_bool g_appRunning{ true };

		boost::asio::io_context ioContext;

		if (argc == 2)
		{
			const int port = std::stoi(argv[1]);
			TV_APP_LOG("Starting Station on port " << port);

			tv_app::Station station(ioContext, port);
			std::thread ioThread([&] { ioContext.run(); });

			tv_app::AudioCapturer audioCap(
				[&station](const std::vector<uint8_t>& data, const std::uint64_t ts) {
					station.BroadcastAudio(data, ts);
				});

			tv_app::VideoCapturer videoCap(
				[&station](const std::vector<uint8_t>& data, const std::uint64_t ts, const std::uint32_t id) {
					station.BroadcastVideo(data, ts, id);
				},
				[&station] {
					return station.CalculateRecommendedQuality();
				});

			audioCap.Start();
			videoCap.Start();

			while (g_appRunning)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			videoCap.Stop();
			audioCap.Stop();
			ioContext.stop();
			ioThread.join();
		}
		else if (argc == 3)
		{
			const std::string address = argv[1];
			const int port = std::stoi(argv[2]);
			TV_APP_LOG("Starting Receiver, connecting to " << address << ":" << port);

			tv_app::Receiver receiver(ioContext, address, port);

			std::thread ioThread([&] { ioContext.run(); });

			receiver.RunDisplayLoop();

			g_appRunning = false;
			receiver.Stop();
			ioContext.stop();
			ioThread.join();
		}
		else
		{
			std::cerr << "Usage:\n"
					  << " Station: tv PORT\n"
					  << " Receiver: tv ADDRESS PORT\n";

			return 1;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
	}

	return 0;
}