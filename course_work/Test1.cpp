#include "WaveFile.h"
#include <stdlib.h>
#include "Normalizer.h"

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
	//for norm_2 window_step must be equal window_wide/3
	constexpr size_t window_wide = 3000; //ms
	constexpr size_t window_step = 1000; //ms
	int ln = -1;
	const auto buffer = new short[window_wide * sample_rate * channels / 1000];
	Normalizer normalizer(window_wide, window_step, sample_rate, channels);
	auto i =0;
	while ((ln = in.ReadSamples(buffer, sample_rate)) > 0) {
		try {
			auto normalized = new short[sample_rate * channels];
			if (normalizer.normalize(buffer, normalized)) {
				out.WriteSamples(normalized, sample_rate);
			}
			if(i == 0) {
				out.WriteSamples(buffer, ln);
			} i++;
			delete[] normalized;
		} catch (Normalization_exception &exception) {
			std::cerr << exception.what() << std::endl;
		} 

	}
	//for norm_1
	auto norm_edge = new short[sample_rate * channels * (window_wide - window_step) / 1000];
	normalizer.normalize_edge(norm_edge);
	out.WriteSamples(norm_edge, sample_rate * (window_wide - window_step) / 1000);
	//for norm_2
	/*auto norm_edge = new short[sample_rate * channels * window_step / 1000];
	normalizer.normalize_edge_2(norm_edge);
	out.WriteSamples(norm_edge, sample_rate * window_step/ 1000);*/
	delete[] norm_edge;
	delete[] buffer;
	out.Close();
	in.Close();
	std::cout << "All done\n";
	return 0;
}
