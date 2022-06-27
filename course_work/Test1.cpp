#include "WaveFile.h"
#include <stdlib.h>
#include "Normalizer.h"
#include <iostream>

int main(int argc, char* argv[]) {
	CReadWaveFile in;
	CWriteWaveFile out;
	if (argc < 3) {
		std::cout << "Usage:\r\n>Test1.exe Input.wav Output.wav\n";
		return -1;
	}
	if (!in.OpenFile(argv[1])) {
		std::cout << "Can't open input file\n";
		return -2;
	}
	if (!out.OpenFile(argv[2])) {
		std::cout << "Can't open output file\n";
		return -3;
	}
	const auto sample_rate = in.get_sample_rate();
	const auto channels = in.get_channel_numb();
	const size_t window_step = sample_rate * 40 / 1000;
	const size_t window_wide = ((3 * sample_rate - 1) / window_step + 1) * window_step;
	const size_t offset = 0 * sample_rate * 40 / 1000;
	auto buffer = new short[window_step * channels];
	auto normalized = new short[window_step * channels];
	Normalizer normalizer(window_wide, window_step, offset, sample_rate, channels);
	auto i = 0;
	int ln = -1;
	auto edge_size = 0;
	while ((ln = in.ReadSamples(buffer, static_cast<int>(window_step))) > 0) {
		if (i == 0) {
			normalizer.normalize_begin(buffer, normalized);
			out.WriteSamples(normalized, static_cast<int>(ln));
			i++;
			continue;
		}
		if (normalizer.normalize(buffer, ln, normalized) > 0) {
			out.WriteSamples(normalized, static_cast<int>(ln));
			edge_size = ln;
		}
	}
	auto norm_edge = new short[(window_wide - edge_size - offset) * channels];
	i = static_cast<int>(normalizer.normalize_edge(edge_size, norm_edge));
	out.WriteSamples(norm_edge, i);
	delete[] norm_edge;
	delete[] buffer;
	//delete[] normalized;
	out.Close();
	in.Close();
	//check integrated loudness for normalized 4 fragments//
	CReadWaveFile new_in;
	if (!new_in.OpenFile(argv[2])) {
		std::cout << "Can't open normalized file\n";
		return -2;
	}
	auto st = static_cast<ebur128_state*>(malloc(sizeof(ebur128_state)));
	if (st == nullptr) {
		throw Normalization_exception("allocation error");
	}
	st = ebur128_init(channels, sample_rate, EBUR128_MODE_I);
	buffer = new short[st->samplerate * st->channels * sizeof(short)];
	double loudness;
	while ((ln = new_in.ReadSamples(buffer, sample_rate)) > 0) {
		ebur128_add_frames_short(st, buffer, static_cast<size_t>(ln));
	}
	ebur128_loudness_global(st, &loudness);
	std::cout << "Global loudness: " << loudness << " LUFS" << std::endl;
	delete[] buffer;
	new_in.Close();
	ebur128_destroy(&st);
	free(st);
	std::cout << "All done\n";
	return 0;
}
