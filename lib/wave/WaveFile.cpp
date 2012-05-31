/************************************************************************
 *                                                                      *
 * This file is part of EAR: Evaluation of Acoustics using Ray-tracing. *
 *                                                                      *
 * EAR is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      * 
 * EAR is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with EAR.  If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                      *
 ************************************************************************/

// Adapted from:
// http://www.codeproject.com/KB/audio-video/wave_class_for_playing_and_recording.aspx

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#ifdef _MSC_VER
#include <windows.h>
#endif

#include "WaveFile.h"

void WaveFile::Init() {
	memset(&desc, 0, sizeof(wavedescr));
	memset(&fmt, 0, sizeof(wavefmt));
	data = 0;
	size = 0;
}

WaveFile::WaveFile() {
	Init();
}

WaveFile::WaveFile(const char* fn) {
	Init();
	Load(fn);
}

WaveFile::~WaveFile()
{
	free(data);
}

bool WaveFile::Load(const char* fn)
{
#ifdef _MSC_VER
	int buffer_size = MultiByteToWideChar(CP_UTF8,0,fn,-1,0,0);
	wchar_t* longname = new wchar_t[buffer_size];
	MultiByteToWideChar(CP_UTF8,0,fn,-1,longname,buffer_size);
	FILE* file = _wfopen(longname,TEXT("rb"));
#else
	FILE* file = open(fn,"rb");
#endif
	if (file) {
		// Read .WAV descriptor
		fread(&desc, sizeof(wavedescr), 1, file);

		// Check for valid .WAV file
		if (strncmp(desc.wave, "WAVE", 4) == 0)
		{
			// Read .WAV format
			fread(&fmt, sizeof(wavefmt), 1, file);

			// Check for valid .WAV file
			if ((strncmp(fmt.id, "fmt", 3) == 0) && (fmt.format == 1))
			{
				// Read next chunk
				char id[4];
				unsigned int block_size;
				fread(id, 1, 4, file);
				fread(&block_size, 4, 1, file);
				unsigned int offset = ftell(file);

				// Read .WAV data
				while (offset < desc.size)
				{
					// Check for .WAV data chunk
					if (strncmp(id, "data", 4) == 0)
					{
						data = (char*)realloc(data, (size+block_size));
						fread(data+size, 1, block_size, file);
						size += block_size;
					} else {
						fseek(file,block_size,SEEK_CUR);
					}

					// Read next chunk
					fread(id, 1, 4, file);
					fread(&size, 4, 1, file);
					offset = ftell(file);
				}
			}
		}

		// Close .WAV file
		fclose(file);
	}

	if ( size ) {
		const int bytes_per_sample = fmt.bitsPerSample >> 3;
		sample_size = size / bytes_per_sample / fmt.channels;
	} else {
		sample_size = 0;
	}

	return true;
}

bool WaveFile::Save(const char* fn)
{
#ifdef _MSC_VER
	int buffer_size = MultiByteToWideChar(CP_UTF8,0,fn,-1,0,0);
	wchar_t* longname = new wchar_t[buffer_size];
	MultiByteToWideChar(CP_UTF8,0,fn,-1,longname,buffer_size);
	FILE* file = _wfopen(longname,TEXT("wb"));
#else
	FILE* file = open(fn,"wb");
#endif
	if (file)
	{
		// Save .WAV descriptor
		desc.size = size;
		fwrite(&desc, sizeof(wavedescr), 1, file);

		// Save .WAV format
		fwrite(&fmt, sizeof(wavefmt), 1, file);

		// Write .WAV data
		fwrite("data", 1, 4, file);
		fwrite(&size, 4, 1, file);
		fwrite(data, 1, size, file);

		// Close .WAV file
		fclose(file);
	}

	return true;
}

float* WaveFile::ToFloat() {
	// Return 0 if format is not understood or the data is empty
	if ( fmt.bitsPerSample != 8 && fmt.bitsPerSample != 16 &&
		fmt.bitsPerSample != 24 )
		return 0;
	if ( ! size || ! data ) return 0;

	const int bytes_per_sample = fmt.bitsPerSample >> 3;
	const float max_sample = (float) (2<<(fmt.bitsPerSample-2));
	float* f = new float[sample_size];
	char* ptr = data;
	for ( unsigned int i = 0; i < sample_size; ++ i) {
		f[i] = 0.0f;
		for ( int c = 0; c < fmt.channels; ++ c ) {
			float current_sample = 0.f;
			if ( bytes_per_sample == 1) {			
				current_sample = *((unsigned char*)ptr)-128.0f;
			} else if ( bytes_per_sample == 2 ) {
				current_sample = *(short*)ptr;
			} else if ( bytes_per_sample == 3 ) {
				const unsigned char c1 = *((unsigned char*)(ptr+0));
				const unsigned char c2 = *((unsigned char*)(ptr+1));
				const unsigned char c3 = *((unsigned char*)(ptr+2));
				current_sample = (float)(c1|(c2<<8)|(c3<<16)|((c3&0x80)?(0xff<<24):(0)));
			}
			ptr += bytes_per_sample;
			f[i] += (float) current_sample / max_sample;
		}
		f[i] /= fmt.channels;			
	}
	return f;
}

bool WaveFile::FromFloat(const float* f, int length, bool norm, float max) {
	data = new char[length*2];
	size = length * 2;
	short* s = (short*) data;
	if ( norm ) {
		if ( max < 0 ) {
			max = -1e9;
			for (long i=0; i<length; i++) {
				if ( f[i] > max ) max = f[i];
			}
			max /= 0.8f;
		} else {
			max /= 0.95f;
		}
	} else {
		max = 1.0f;
	}
	for (long i=0; i<length; i++) {
		s[i] = (short) (f[i] / max * 32768.0f);
	}
	memcpy(desc.riff,"RIFF",4);
	memcpy(desc.wave,"WAVE",4);
	fmt.bitsPerSample = 16;
	fmt.blockAlign = 2;
	fmt.byteRate = 88200;
	fmt.channels = 1;
	fmt.format = 1;
	memcpy(fmt.id,"fmt ",4);
	fmt.sampleRate = 44100;
	fmt.size = 16;
	return true;
}

bool WaveFile::FromFloat(const float* left, const float* right, int length1, int length2, bool norm) {
	int length = (std::max)(length1,length2);
	data = new char[length*4];
	size = length * 4;
	short* s = (short*) data;
	float max = -1e9;
	if ( norm ) {
		for (long i=0; i<length1; i++) {
			if ( left[i] > max ) max = left[i];
		}
		for (long i=0; i<length2; i++) {
			if ( right[i] > max ) max = right[i];
		}
		max /= 0.8f;
	} else {
		max = 1.0f;
	}
	for (long i=0; i<length; i++) {
		s[2*i] = (short) (i < length1 ? (left[i] / max * 32768.0f) : 0);
		s[2*i+1] = (short) (i < length2 ? (right[i] / max * 32768.0f) : 0);
	}
	memcpy(desc.riff,"RIFF",4);
	memcpy(desc.wave,"WAVE",4);
	fmt.bitsPerSample = 16;
	fmt.blockAlign = 4;
	fmt.byteRate = 88200;
	fmt.channels = 2;
	fmt.format = 1;
	memcpy(fmt.id,"fmt ",4);
	fmt.sampleRate = 44100;
	fmt.size = 16;
	return true;
}