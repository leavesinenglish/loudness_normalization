// Test1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include <iostream>
#include "WaveFile.h"
#include "ebur128.h"
#include <stdlib.h>
#include <string.h>
#include <vector>

int main(int argc, char* argv[]) {
	auto st = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
	double loudness;
	if (argc < 3) {
		std::cout << "Usage:\r\n>Test1.exe Input.wav Output.wav\n";
		return -1;
	}
	CReadWaveFile in;
	CWriteWaveFile out;
	if (!in.OpenFile(argv[1])) {
		std::cout << "Can't open input file\n";
		return -2;
	}
	if (!out.OpenFile(argv[2])) {
		std::cout << "Can't open output file\n";
		return -3;
	}
	const auto c_sample_count = in.get_sample_rate();
	int ln = -1;
	st = ebur128_init(static_cast<unsigned>(in.get_channel_numb()),
	                  static_cast<unsigned>(in.get_sample_rate()), EBUR128_MODE_I);
	if (!st) {
		fprintf(stderr, "Could not create ebur128_state!\n");
		return 1;
	}

	if (in.get_channel_numb() == 5) {
		ebur128_set_channel(st, 0, EBUR128_LEFT);
		ebur128_set_channel(st, 1, EBUR128_RIGHT);
		ebur128_set_channel(st, 2, EBUR128_CENTER);
		ebur128_set_channel(st, 3, EBUR128_LEFT_SURROUND);
		ebur128_set_channel(st, 4, EBUR128_RIGHT_SURROUND);
	}
	const auto buffer = new short[st->samplerate * st->channels * sizeof(short)];
	std::vector<double> loudnesses;
	constexpr unsigned long window = 300;
	ebur128_set_max_window(st, 500);
	while ((ln = in.ReadSamples(buffer, c_sample_count * window / 1000)) > 0) {
		ebur128_add_frames_short(st, buffer, static_cast<size_t>(ln));
		if (ebur128_loudness_window(st, window, &loudness) != EBUR128_SUCCESS) {
			std::cout << "FAILURE" << std::endl;
			delete[] buffer;
			out.Close();
			in.Close();
			ebur128_destroy(&st);
			free(st);
			return 1;
		}
		loudnesses.push_back(loudness);
		//here will be normalization proc 
	}

	int i = 0;
	for (auto loudness : loudnesses) {
		++i;
		//std::cout << i << " loudness within" << window << " ms" << loudness << " LUFS" << std::endl;
		const int a = 60000 / window / 4;
		if (i % a == 0) {
			double sum = 0;
			for (auto j = i - a; j != i; j++) {
				sum += loudnesses[j];
			}
			sum /= a;
			std::cout << '\n' << "Mean loudness for a period: " << sum << " LUFS" << std::endl << '\n';
		}
	}
	delete[] buffer;
	out.Close();
	in.Close();
	ebur128_destroy(&st);
	free(st);
	std::cout << "All done\n";
	return 0;
}
