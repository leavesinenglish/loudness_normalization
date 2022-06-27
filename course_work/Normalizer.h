#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "ebur128.h"

class Normalization_exception final : std::exception {
	std::string error;
public:
	Normalization_exception(std::string error) : error(std::move(error)) {}
	const char* what() const noexcept override { return error.c_str(); }
};

class Normalizer {
	std::vector<short> window_storage;
	double previous_volume; //linear
	const size_t window_wide; //samples
	const size_t window_step; //samples
	const size_t offset; //samples
	WORD channels;
	int calls;
	ebur128_state* st_global;
	const double target_lufs = -23.0;
	size_t sample_rate;
public:
	Normalizer(const size_t wide_in_samples, size_t step_in_samples, const size_t off, const DWORD sample_rate, const WORD channels_num) :
		window_wide(wide_in_samples), window_step(step_in_samples), offset(off), channels(channels_num), sample_rate(sample_rate) {
		previous_volume = -1.0;
		st_global = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
	}

	~Normalizer() {
		ebur128_destroy(&st_global);
	}

	size_t normalize(const short* source, const size_t size, short* result) {
		// add new data to ebur128
		for (auto i = 0; i < size * channels; i++) {
			window_storage.push_back(source[i]);
		}
		//ebur128_add_frames_short(st_global, source, size);
		if (window_storage.size() < window_wide * channels) {
			// need more data
			return 0;
		}
		auto st = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
		if (st == nullptr) {
			throw Normalization_exception("allocation error");
		}
		st = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
		ebur128_add_frames_short(st, window_storage.data(), window_wide);
		// calc loudness
		double current_loudness;

		if (ebur128_loudness_global(st, &current_loudness) != EBUR128_SUCCESS) {
			throw Normalization_exception("cannot calculate loudness");
		}
		// convert loudness difference to linear volume
		double current_volume = pow(10.0, (target_lufs - current_loudness) / 20.0);
		if (previous_volume < 0) {
			previous_volume = current_volume;
		}
		// interpolate volume for output data
		const double f_step = (current_volume - previous_volume) / window_step;
		int n_pos = 0;
		for (auto i = 0; i < size; i++) {
			for (int j = 0; j < channels; j++) {
				result[n_pos] = static_cast<short>(previous_volume * window_storage[n_pos + offset]);
				n_pos++;
			}
			previous_volume += f_step;
		}
		// remove output data
		window_storage.erase(window_storage.begin(), window_storage.begin() + window_step * channels);
		// all done
		ebur128_destroy(&st);
		free(st);
		return size;
	}

	size_t normalize_edge(const size_t size, short* result) const {
		auto a = window_wide - size - offset;
		auto b = (size + offset - window_step) * channels;
		for (auto i = 0; i < (window_wide - size - offset) * channels; i++) {
			result[i] = static_cast<short>(previous_volume * window_storage[(size + offset - window_step) * channels + i]);
		}
		return window_wide - size - offset;
	}

	void normalize_begin(const short* begin, short* result) {
		auto st = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
		if (st == nullptr) {
			throw Normalization_exception("allocation error");
		}
		st = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
		for (auto i = 0; i < offset * channels; i++) {
			window_storage.push_back(begin[i]);
		}
		ebur128_add_frames_short(st, begin, offset);
		double loudness = 0;
		if (ebur128_loudness_global(st, &loudness) != EBUR128_SUCCESS) {
			//ebur128_destroy(&st);
			//free(st);
			throw Normalization_exception("cannot calculate loudness");
		}
		const double volume = pow(10.0, (target_lufs - loudness) / 20.0);
		for (auto i = 0; i < offset * channels; i++) {
			result[i] = static_cast<short>(volume * begin[i]);
		}
		ebur128_destroy(&st);
		free(st);
	}
};
