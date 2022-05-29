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
	double previous_loudness; //LUFS
	const size_t window_wide; //ms
	size_t window_step; //ms
	size_t sample_rate;
	size_t channels;
public:
	Normalizer(const size_t wide, const size_t step, const size_t sample_rate, const size_t channels_num) :
		window_wide(wide), window_step(step), sample_rate(sample_rate), channels(channels_num) {
		previous_loudness = 1;
	}

	bool normalize(short* source, short* result) {
		//moving window
		for (auto i = 0; i < window_step * sample_rate * channels / 1000; i++) {
			window_storage.push_back(source[i]);
		}
		if (window_storage.size() < window_wide * sample_rate * channels / 1000) {
			return false;
		}
		if (window_storage.size() > window_wide * sample_rate * channels / 1000) {
			window_storage.erase(window_storage.begin(),
			                     window_storage.begin() + window_step * sample_rate * channels / 1000);
		}
		double current_loudness;
		auto st = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
		if (st == nullptr) {
			throw Normalization_exception("allocation error");
		}
		st = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
		ebur128_add_frames_short(st, window_storage.data(), window_storage.size() / channels);
		if (ebur128_loudness_global(st, &current_loudness) != EBUR128_SUCCESS) {
			ebur128_destroy(&st);
			free(st);
			throw Normalization_exception("cannot calculate loudness");
		}
		if (previous_loudness > 0) {
			previous_loudness = current_loudness;
		}
		//linear interpolation for coeff of changing loudness:
		for (auto i = 0; i < sample_rate * window_step * channels / 1000; i++) {
			double coeff = 0;
			coeff = previous_loudness + (current_loudness - previous_loudness) * (i) / (sample_rate * window_wide *
				channels / 1000);
			coeff /= -23; //normalized to the target level
			coeff = exp((0.691 + coeff) * log(10) / 20); // / exp((0.691 - 23) * log(10) / 20);
			result[i] = coeff * window_storage[i];
		}

		ebur128_destroy(&st);
		free(st);
		return true;
	}

	bool normalize_2(short* source, short* result) {
		window_step = window_wide / 3;
		for (auto i = 0; i < window_step * sample_rate * channels / 1000; i++) {
			window_storage.push_back(source[i]);
		}
		if (window_storage.size() < window_wide * sample_rate * channels / 1000) {
			return false;
		}
		if (window_storage.size() > window_wide * sample_rate * channels / 1000) {
			window_storage.erase(window_storage.begin(),
			                     window_storage.begin() + window_step * sample_rate * channels / 1000);
		}
		double right_loudness, left_loudness;
		auto st = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
		if (st == nullptr) {
			throw Normalization_exception("allocation error");
		}
		auto st_1 = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
		if (st_1 == nullptr) {
			throw Normalization_exception("allocation error");
		}
		st = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
		st_1 = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
		for (auto i = 0; i < window_step * sample_rate * channels / 1000; i++) {
			result[i] = window_storage[i];
		}
		ebur128_add_frames_short(st, result, window_step * sample_rate / 1000);
		if (ebur128_loudness_global(st, &left_loudness) != EBUR128_SUCCESS) {
			ebur128_destroy(&st);
			free(st);
			throw Normalization_exception("cannot calculate loudness");
		}
		for (auto i = 0; i < window_step * sample_rate * channels / 1000; i++) {
			result[i] = window_storage[window_storage.size() - window_step * sample_rate * channels / 1000 + i];
		}
		ebur128_add_frames_short(st_1, result, window_step * sample_rate / 1000);
		if (ebur128_loudness_global(st_1, &right_loudness) != EBUR128_SUCCESS) {
			ebur128_destroy(&st_1);
			free(st_1);
			throw Normalization_exception("cannot calculate loudness");
		}
		for (auto i = 0; i < sample_rate * window_step * channels / 1000; i++) {
			double coeff = 0;
			coeff = left_loudness + (right_loudness - left_loudness) * (i + window_step * sample_rate * channels / 1000)
				/ (sample_rate * window_wide * channels / 1000);
			coeff /= -23;
			coeff = exp((0.691 + coeff) * log(10) / 20);
			result[i] = coeff * window_storage[i + window_step * sample_rate * channels / 1000];
		}
		ebur128_destroy(&st);
		free(st);
		ebur128_destroy(&st_1);
		free(st_1);
		return true;
	}

	void normalize_edge(short* result) {
		double current_loudness;
		auto st = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
		if (st == nullptr) {
			throw Normalization_exception("allocation error");
		}
		st = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
		window_storage.erase(window_storage.begin(),
		                     window_storage.begin() + window_step * sample_rate * channels / 1000);
		ebur128_add_frames_short(st, window_storage.data(), window_storage.size() / channels);
		if (ebur128_loudness_global(st, &current_loudness) != EBUR128_SUCCESS) {
			ebur128_destroy(&st);
			free(st);
			throw Normalization_exception("cannot calculate loudness");
		}
		if (previous_loudness > 0) {
			previous_loudness = current_loudness;
		}
		for (auto i = 0; i < window_storage.size(); i++) {
			double coeff = 0;
			coeff = previous_loudness + (current_loudness - previous_loudness) * (i) / (window_storage.size());
			coeff /= -23;
			coeff = exp((0.691 + coeff) * log(10) / 20);
			result[i] = coeff * window_storage[i];
		}
		ebur128_destroy(&st);
		free(st);
	}

	void normalize_edge_2(short* result) {
		for (auto i = 0; i < window_step * sample_rate * channels / 1000; i++) {
			result[i] = window_storage[window_storage.size() - window_step * sample_rate * channels / 1000 + i];
		}
	}

};
