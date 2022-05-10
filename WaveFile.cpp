#include "pch.h"
#include "WaveFile.h"

// constants for WAVE file
#define rifftypeWAVE  mmioFOURCC( 'W','A','V','E' )
#define ckidWAVEfmt   mmioFOURCC( 'f','m','t',' ' )
#define ckidWAVEfact  mmioFOURCC( 'f','a','c','t' )
#define ckidWAVEdata  mmioFOURCC( 'd','a','t','a' )


// class for reading WAVE-file
//  !!! TEST-ONLY !!! file must be STEREO & 16-bits per sample ONLY !!!
bool CReadWaveFile::OpenFile(char* fname) {
	MMCKINFO ck, ckRiff, ckData;
	HMMIO hmmio;

	// open file
	hmmio = mmioOpenA(fname, 0, DWORD(MMIO_READ | MMIO_DENYNONE));
	if (NULL == hmmio)
		return false;
	// obtain file size
	DWORD nFileSize = mmioSeek(hmmio, 0, SEEK_END);
	mmioSeek(hmmio, 0, SEEK_SET);

	// open main 'RIFF' header
	ckRiff.fccType = rifftypeWAVE;
	if (mmioDescend(hmmio, &ckRiff, NULL, MMIO_FINDRIFF)) {
		mmioClose(hmmio, 0);
		return false;
	}

	// read format
	ck.ckid = ckidWAVEfmt;
	if (mmioDescend(hmmio, &ck, &ckRiff, MMIO_FINDCHUNK)
		|| ck.cksize > nFileSize - ck.dwDataOffset) {
		mmioClose(hmmio, 0);
		return false;
	}
	WAVEFORMATEX format;
	mmioRead(hmmio, HPSTR(&format), min(ck.cksize, sizeof(format)));
	mmioAscend(hmmio, &ck, 0);

	// find 'data'
	ckData.ckid = ckidWAVEdata;
	if (mmioDescend(hmmio, &ckData, &ckRiff, MMIO_FINDCHUNK)) {
		mmioClose(hmmio, 0);
		return false;
	}
	mmioClose(hmmio, 0);

	// check format: file must be STEREO & 16-bits per sample ONLY
	if (WAVE_FORMAT_PCM != format.wFormatTag
		|| 16 != format.wBitsPerSample
		|| 2 != format.nChannels
		|| 4 != format.nBlockAlign) {
		return false;
	}

	// open file for data reading
	fopen_s(&m_hFile, fname, "rb");
	if (nullptr == m_hFile)
		return false;

	// seek to begin of data
	fseek(m_hFile, ckData.dwDataOffset, SEEK_SET);
	m_nSampleCount = ckData.cksize;
	channel_numb = format.nChannels;
	sample_rate = format.nSamplesPerSec;
	return true;
}

void CReadWaveFile::Close() {
	if (NULL != m_hFile) {
		fclose(m_hFile);
		m_hFile = NULL;
	}
	m_nSampleCount = 0;
}

int CReadWaveFile::ReadSamples(short int* buffer, int max_sample_cout) {
	if (m_nSampleCount <= 0)
		return 0;
	int cnt = min(m_nSampleCount, max_sample_cout);
	cnt = fread(buffer, 4, cnt, m_hFile);
	m_nSampleCount -= cnt;
	return cnt;
}

size_t CReadWaveFile::get_sample_rate() {
	return sample_rate;
}

size_t CReadWaveFile::get_channel_numb() {
	return channel_numb;
}


// class for writing WAVE-file
//  !!! TEST-ONLY !!! file may be STEREO & 16-bits per sample ONLY !!!
bool CWriteWaveFile::OpenFile(char* fname) {
	ZeroMemory(&ckRiff, sizeof(MMCKINFO));

	// open output file
	m_hFile = mmioOpenA(fname, NULL, MMIO_CREATE | MMIO_WRITE);
	if (NULL == m_hFile)
		return false;

	// create main RIFF chunk
	ckRiff.fccType = rifftypeWAVE;
	if (mmioCreateChunk(m_hFile, &ckRiff, MMIO_CREATERIFF))
		return false;

	// create chunk with audio format
	ck.ckid = ckidWAVEfmt;
	if (mmioCreateChunk(m_hFile, &ck, 0))
		return false;
	// save audio format
	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = 48000;
	format.nAvgBytesPerSec = 48000 * 4;
	format.nBlockAlign = 4;
	format.wBitsPerSample = 16;
	format.cbSize = 0;
	if (mmioWrite(m_hFile, (LPSTR)&format, sizeof(format)) == -1)
		return false;
	if (mmioAscend(m_hFile, &ck, 0))
		return false;

	// open chunk for data
	ZeroMemory(&ck, sizeof(MMCKINFO));
	ck.ckid = ckidWAVEdata;
	if (mmioCreateChunk(m_hFile, &ck, 0))
		return false;

	// ready to write data
	m_nSampleCnt = 0;
	return true;
}

void CWriteWaveFile::Close() {
	if (m_nSampleCnt > 0) {
		mmioAscend(m_hFile, &ck, 0);
		mmioAscend(m_hFile, &ckRiff, 0);
	}
	m_nSampleCnt = -2;
	if (NULL != m_hFile) {
		mmioClose(m_hFile, 0);
		m_hFile = NULL;
	}
}

bool CWriteWaveFile::WriteSamples(short int* buffer, int sample_cout) {
	if (m_nSampleCnt < 0)
		return false;
	int size = mmioWrite(m_hFile, (HPSTR)buffer, sample_cout * 4);
	if (size < 0)
		return false;
	// all right
	m_nSampleCnt += size / 4;
	return true;
}
