#pragma once

#include "detection.h"
#include <vector>
#include <random>

struct ShakingWindow {
	HWND handle;
	RECT initial_position;
};

class Shaker {
  public:
	void Start();
	void Stop();
	void SetEnabled(bool enabled);
	bool IsEnabled() const;
	bool IsRunning() const {
		return running_;
	}

  private:
	static void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);
	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lparam);
	static bool IsShakeable(HWND hwnd);

	void Tick();
	void EnumerateWindows();
	void ShakeWindows();
	void RestoreWindows();

	bool running_ = false;
	UINT_PTR timer_id_ = 0;

	visualisation_stream_v2::ptr vis_stream_;
	BeatDetector detector_;

	std::vector<ShakingWindow> windows_;
	std::mt19937 rng_{std::random_device{}()};
};
