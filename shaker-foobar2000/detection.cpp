#include "stdafx.h"
#include "detection.h"

BeatDetector::BeatDetector(int blocks_per_second)
	: blocks_per_second_(blocks_per_second), energy_buffer_(blocks_per_second, 0.0), energy_index_(0) {}

bool BeatDetector::DigestSpectrum(const audio_sample* spectrum, size_t sample_count) {
	double block_energy = 0.0;
	for (size_t i = 0; i < sample_count; i++) {
		block_energy += spectrum[i];
	}

	double average_energy = 0.0;
	for (int i = 0; i < blocks_per_second_; i++) {
		average_energy += energy_buffer_[i];
	}
	average_energy /= blocks_per_second_;

	energy_buffer_[energy_index_ % blocks_per_second_] = block_energy;
	energy_index_++;

	return block_energy > average_energy;
}
