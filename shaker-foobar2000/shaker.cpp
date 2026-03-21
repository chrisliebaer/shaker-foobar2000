#include "stdafx.h"
#include "shaker.h"
#include <dwmapi.h>

// {66AB12C8-D37D-4C6E-B50B-D4C091F6E963}
static const GUID guid_cfg_enabled = {0x66ab12c8, 0xd37d, 0x4c6e, {0xb5, 0x0b, 0xd4, 0xc0, 0x91, 0xf6, 0xe9, 0x63}};
static cfg_bool cfg_enabled(guid_cfg_enabled, true);

static Shaker* g_shaker = nullptr;

static constexpr int SHAKE_DISTANCE = 8;
static constexpr int TIMER_INTERVAL_MS = 1000 / 60;
static constexpr unsigned FFT_SIZE = 1024;

bool Shaker::IsShakeable(HWND hwnd) {
	if (!IsWindowVisible(hwnd))
		return false;

	WINDOWINFO wi = {};
	wi.cbSize = sizeof(WINDOWINFO);
	if (!GetWindowInfo(hwnd, &wi))
		return false;
	if (wi.dwExStyle & WS_EX_TOOLWINDOW)
		return false;

	int cloaked = 0;
	DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
	if (cloaked)
		return false;

	return true;
}

BOOL CALLBACK Shaker::EnumWindowsProc(HWND hwnd, LPARAM lparam) {
	auto* windows = reinterpret_cast<std::vector<ShakingWindow>*>(lparam);

	if (!IsShakeable(hwnd))
		return TRUE;

	ShakingWindow sw;
	sw.handle = hwnd;
	if (!GetWindowRect(hwnd, &sw.initial_position))
		return TRUE;

	windows->push_back(sw);
	return TRUE;
}

void Shaker::EnumerateWindows() {
	windows_.clear();
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows_));
}

void Shaker::ShakeWindows() {
	std::uniform_int_distribution<int> dist(0, 1);

	for (auto& sw : windows_) {
		if (!IsWindow(sw.handle))
			continue;

		RECT rc;
		if (!GetWindowRect(sw.handle, &rc))
			continue;

		int dx = SHAKE_DISTANCE * (dist(rng_) ? 1 : -1);
		int dy = SHAKE_DISTANCE * (dist(rng_) ? 1 : -1);

		SetWindowPos(sw.handle, nullptr, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void Shaker::RestoreWindows() {
	for (auto& sw : windows_) {
		if (!IsWindow(sw.handle))
			continue;
		SetWindowPos(sw.handle, nullptr, sw.initial_position.left, sw.initial_position.top, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void Shaker::Tick() {
	if (!vis_stream_.is_valid())
		return;

	double time;
	if (!vis_stream_->get_absolute_time(time))
		return;

	audio_chunk_impl_temporary chunk;
	if (!vis_stream_->get_spectrum_absolute(chunk, time, FFT_SIZE))
		return;

	const audio_sample* data = chunk.get_data();
	size_t count = chunk.get_used_size();

	if (detector_.DigestSpectrum(data, count)) {
		ShakeWindows();
	}
}

void CALLBACK Shaker::TimerProc(HWND, UINT, UINT_PTR, DWORD) {
	if (g_shaker)
		g_shaker->Tick();
}

bool Shaker::IsEnabled() const {
	return cfg_enabled;
}

void Shaker::SetEnabled(bool enabled) {
	cfg_enabled = enabled;
	if (!enabled)
		Stop();
}

void Shaker::Start() {
	if (running_ || !cfg_enabled)
		return;

	try {
		visualisation_manager::ptr mgr;
		mgr = visualisation_manager::get();
		visualisation_stream::ptr raw;
		mgr->create_stream(raw, visualisation_manager::KStreamFlagNewFFT);
		raw->service_query_t(vis_stream_);
	} catch (...) {
		FB2K_console_formatter() << "Shaker: failed to create visualisation stream";
		return;
	}

	if (vis_stream_.is_valid()) {
		vis_stream_->set_channel_mode(visualisation_stream_v2::channel_mode_mono);
	}

	detector_ = BeatDetector(60);
	EnumerateWindows();

	g_shaker = this;
	timer_id_ = SetTimer(nullptr, 0, TIMER_INTERVAL_MS, TimerProc);
	running_ = true;

	FB2K_console_formatter() << "Shaker: started (" << windows_.size() << " windows)";
}

void Shaker::Stop() {
	if (!running_)
		return;

	if (timer_id_) {
		KillTimer(nullptr, timer_id_);
		timer_id_ = 0;
	}

	RestoreWindows();
	windows_.clear();
	vis_stream_.release();
	g_shaker = nullptr;
	running_ = false;

	FB2K_console_formatter() << "Shaker: stopped";
}

namespace {

static Shaker g_shaker_instance;

// {47220C1A-D068-49AA-9EA5-A1ED5F497D98}
static const GUID guid_shaker_toggle = {0x47220c1a, 0xd068, 0x49aa, {0x9e, 0xa5, 0xa1, 0xed, 0x5f, 0x49, 0x7d, 0x98}};

class ShakerMainMenu : public mainmenu_commands {
  public:
	t_uint32 get_command_count() override {
		return 1;
	}
	GUID get_command(t_uint32) override {
		return guid_shaker_toggle;
	}
	void get_name(t_uint32, pfc::string_base& out) override {
		out = "Shaker";
	}
	bool get_description(t_uint32, pfc::string_base& out) override {
		out = "Toggle window shaking on beat";
		return true;
	}
	GUID get_parent() override {
		return mainmenu_groups::playback;
	}

	void execute(t_uint32, service_ptr_t<service_base>) override {
		g_shaker_instance.SetEnabled(!g_shaker_instance.IsEnabled());
		if (g_shaker_instance.IsEnabled() && static_api_ptr_t<playback_control>()->is_playing() &&
			!static_api_ptr_t<playback_control>()->is_paused()) {
			g_shaker_instance.Start();
		}
	}

	bool get_display(t_uint32 p_index, pfc::string_base& p_text, t_uint32& p_flags) override {
		bool rv = mainmenu_commands::get_display(p_index, p_text, p_flags);
		if (rv && g_shaker_instance.IsEnabled())
			p_flags |= flag_checked;
		return rv;
	}
};

static mainmenu_commands_factory_t<ShakerMainMenu> g_shaker_menu;

class ShakerInitQuit : public initquit {
  public:
	void on_init() override {}
	void on_quit() override {
		g_shaker_instance.Stop();
	}
};

class ShakerPlayCallback : public play_callback_static {
  public:
	unsigned get_flags() override {
		return flag_on_playback_starting | flag_on_playback_stop | flag_on_playback_pause;
	}

	void on_playback_starting(play_control::t_track_command, bool p_paused) override {
		if (!p_paused)
			g_shaker_instance.Start();
	}

	void on_playback_stop(play_control::t_stop_reason) override {
		g_shaker_instance.Stop();
	}

	void on_playback_pause(bool p_state) override {
		if (p_state)
			g_shaker_instance.Stop();
		else
			g_shaker_instance.Start();
	}

	void on_playback_new_track(metadb_handle_ptr) override {}
	void on_playback_seek(double) override {}
	void on_playback_edited(metadb_handle_ptr) override {}
	void on_playback_dynamic_info(const file_info&) override {}
	void on_playback_dynamic_info_track(const file_info&) override {}
	void on_playback_time(double) override {}
	void on_volume_change(float) override {}
};

FB2K_SERVICE_FACTORY(ShakerInitQuit);
FB2K_SERVICE_FACTORY(ShakerPlayCallback);

} // namespace
