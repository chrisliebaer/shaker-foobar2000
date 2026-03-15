#pragma once

#include <vector>
#include <cstddef>

// Detects beats by comparing per-frame spectrum energy against a
// rolling average over the last ~1 second of frames.
class BeatDetector {
  public:
	explicit BeatDetector(int blocks_per_second = 60);

	// Returns true if the current frame's energy exceeds the rolling average.
	bool DigestSpectrum(const audio_sample* spectrum, size_t sample_count);

  private:
	int blocks_per_second_;
	std::vector<double> energy_buffer_;
	size_t energy_index_ = 0;
};
