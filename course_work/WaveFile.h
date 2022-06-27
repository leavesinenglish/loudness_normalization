#pragma once
#include "pch.h"
// class for reading WAVE-file
//  !!! TEST-ONLY !!! file must be STEREO & 16-bits per sample ONLY !!!
class CReadWaveFile {
public:
	CReadWaveFile() {}

	~CReadWaveFile() { Close(); }

	bool OpenFile(char* fname);

	void Close();

	int ReadSamples(short int* buffer, int max_sample_count);

	size_t get_sample_rate();

	size_t get_channel_numb();

private:
	FILE* m_hFile = NULL;
	int m_nSampleCount = 0;
	size_t sample_rate;
	size_t channel_numb;
};

// class for writing WAVE-file
//  !!! TEST-ONLY !!! file may be STEREO & 16-bits per sample ONLY !!!
class CWriteWaveFile {
public:
	CWriteWaveFile() {}

	~CWriteWaveFile() { Close(); }

	bool OpenFile(char* fname);

	void Close();

	bool WriteSamples(short int* buffer, int sample_count);

private:
	HMMIO m_hFile = NULL;
	MMCKINFO ckRiff, ck;
	int m_nSampleCnt = -1;
};
