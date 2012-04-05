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

#ifndef _WAVE_H
#define _WAVE_H

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	char riff[4];
	unsigned int size;
	char wave[4];

} wavedescr;
typedef struct
{
	char id[4];
	unsigned int size;
	short format;
	short channels;
	unsigned int sampleRate;
	unsigned int byteRate;
	short blockAlign;
	short bitsPerSample;

} wavefmt;
#pragma pack(pop)

class WaveFile
{
private:
	wavedescr desc;
	wavefmt fmt;
	char* data;
	unsigned int size;
	unsigned int sample_size;
	void Init();
public:
	WaveFile();
	WaveFile(const char* fn);
	~WaveFile();
	bool Load(const char* fn);
	bool Save(const char* fn);
	float* ToFloat();
	bool FromFloat(const float*, int size, bool norm = false, float norm_max = -1.0f);
	bool FromFloat(const float* left, const float* right, int size1, int size2, bool norm = false);
	
	bool HasData() {return data != 0; }
	void* GetData() {return data;}
	unsigned int GetSize()  {return size;}
	unsigned int GetSampleSize() {return sample_size;}
	short GetChannels() {return fmt.channels;}
	unsigned int GetSampleRate() {return fmt.sampleRate;}
	short GetBitsPerSample() {return fmt.bitsPerSample;}
};

#endif