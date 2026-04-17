#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

#include <opencv2/opencv.hpp>

#include <Common.hpp>

namespace tv_app
{

class VideoCapturer
{
public:
	using FrameReadyCallback = std::function<void(const std::vector<uchar>&, uint64_t, uint32_t)>;
	using QualityRequestCallback = std::function<int()>;

	VideoCapturer(FrameReadyCallback onFrame, QualityRequestCallback getQuality)
		: m_onFrame(std::move(onFrame))
		, m_getQuality(std::move(getQuality))
		, m_frameId(0)
		, m_running(false)
	{
	}

	~VideoCapturer() { Stop(); }

	void Start()
	{
		m_running = true;
		m_captureThread = std::thread(&VideoCapturer::CaptureLoop, this);
	}

	void Stop()
	{
		m_running = false;
		if (m_captureThread.joinable())
			m_captureThread.join();
	}

private:
	void CaptureLoop()
	{
		cv::VideoCapture capture(0);
		if (!capture.isOpened())
		{
			TV_APP_LOG("Failed to open webcam");
			return;
		}

		cv::Mat frame;
		std::vector<uchar> buffer;

		while (m_running)
		{
			capture >> frame;
			if (frame.empty())
			{
				continue;
			}

			const int quality = m_getQuality ? m_getQuality() : defaultQuality;
			std::vector params = { cv::IMWRITE_JPEG_QUALITY, quality };

			cv::imencode(".jpg", frame, buffer, params);

			if (m_onFrame)
			{
				m_onFrame(buffer, GetCurrentTimestamp(), ++m_frameId);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(30)); // ~30 FPS
		}
	}

	FrameReadyCallback m_onFrame;
	QualityRequestCallback m_getQuality;
	uint32_t m_frameId;
	std::atomic_bool m_running;
	std::thread m_captureThread;
};

} // namespace tv_app