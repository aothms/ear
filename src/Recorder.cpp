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

#include <vector>
#include <stdio.h>
#include <string.h>
#include <algorithm>

#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>
#include <gmtl/Point.h>

#include "Recorder.h"
#include "Animated.h"
#include "SoundFile.h"

void FloatBuffer::resizeArray(const unsigned int l) {
  if ( l <= length ) return;
	const float* old_data = data;
	data = new float[l];
	memcpy(data,old_data,length*sizeof(data[0]));
	memset(data+length,0,(l-length)*sizeof(data[0]));
	length = l;
	delete[] old_data;
}

FloatBuffer::FloatBuffer() {
	data = 0;
	real_length = length = 0;
	first_sample = INITIAL_BUFFER_SIZE -1;
	this->resizeArray(INITIAL_BUFFER_SIZE);
}
FloatBuffer::~FloatBuffer() {
	delete data;
}
float& FloatBuffer::operator[] (const unsigned int i) {
	if ( i >= length ) {
		resizeArray(i+INCREMENTAL_BUFFER_SIZE);
	}
	if ( i > real_length ) real_length = i;
	if ( i < first_sample ) first_sample = i;
	return data[i];
}
const float& FloatBuffer::operator[] (const unsigned int i) const {
	if ( i >= length ) {
		return zero;
	}
	return data[i];
}

float FloatBuffer::RootMeanSquare() const {
	if ( ! real_length ) return 0;
	float x = 0.0;
	for ( unsigned int i = first_sample; i < real_length; i ++ ) {
		x += data[i]*data[i];
	}
	return sqrt(x/(float)real_length);
}

float FloatBuffer::Maximum() const {
	float x = 0.0;
	for ( unsigned int i = first_sample; i < real_length; i ++ ) {
		const float a = fabs(data[i]);
		if ( a > x ) x = a;
	}
	return x;
}

void FloatBuffer::Multiply(float f) {
	for ( unsigned int i = first_sample; i < real_length; i ++ ) {
		data[i] *= f;
	}
}

void FloatBuffer::Normalize(float M, float MAX) {
	Multiply( M / (MAX < 0 ? Maximum() : MAX) );
}

void FloatBuffer::Truncate(unsigned int l) {
	if ( l == 0 ) l = 1;
	if ( l >= length ) {
		resizeArray(l);
	}
	real_length = l;
}

void FloatBuffer::Power(float a) {
	for ( unsigned int i = first_sample; i < real_length; i ++ ) {
		const float f = pow(abs(data[i]),a);
		data[i] = data[i] < 0 ? (f*-1.0f) : f;
	}
}

unsigned int FloatBuffer::getLength(float tresh) const {
	if ( tresh < 0.0f )
		return real_length;
	unsigned int max = 0;
	for ( unsigned int i = first_sample; i < length; ++ i ) {
		if ( abs(data[i]) >= tresh ) max = i;
	}
	return max + 1;
}

void FloatBuffer::Write(const std::string& fn) const {
  std::ofstream f(fn.c_str(),std::ios_base::binary);
	f.write((char*)data,sizeof(float)*(real_length+1));	
}

void FloatBuffer::Read(const std::string& fn) {
  std::ifstream f(fn.c_str(),std::ios_base::binary);
	f.seekg(0,std::ios_base::end);
  std::streamsize stream_size = f.tellg();
	f.seekg(0,std::ios_base::beg);
	resizeArray(stream_size/4);
  f.read((char*)data,stream_size);
  first_sample = 0;
  real_length = stream_size / 4;
}

#ifdef USE_FFTW
// Processes a sound file to include the response in the recorder
// track. The response is not interpolated with a successive
// response. A Fast Fourier Transform is used to transfer both the
// dry signal and the impulse response to the frequency domain to
// reduce the complexity (= speed up) of the convolution operation.
// An additional argument specifies whether to optionally fade in
// or fade out the input signal to help with the interpolation
// of successive key-frames.
RecorderTrack* RecorderTrack::Process(SoundFile* const sound_file,
                                      Fade fade) const {

	const RecorderTrack& _this = *this;
	const unsigned int M = this->getLength();
	const unsigned int N = sound_file->sample_length;
	const unsigned int MN = M+N+1;
	const unsigned int MNh = MN/2+1;

	float* a = (float*) fftwf_malloc(sizeof (float) * MN);	
	memset(a,0,sizeof(float)*MN);
	memcpy(a,&_this[0],sizeof(float)*M);

	fftwf_complex* A = (fftwf_complex *) fftwf_malloc (
		sizeof (fftwf_complex) * MNh);
	memset(A,0,sizeof (fftwf_complex) * MNh);
	fftwf_plan fft_plan1 = fftwf_plan_dft_r2c_1d(
		MN,a,A,FFTW_ESTIMATE);
	fftwf_execute(fft_plan1);
	fftwf_free(a);

	float* b = (float*) fftwf_malloc(sizeof (float) * MN);
	memset(b,0,sizeof(float)*MN);
	memcpy(b,sound_file->data,sizeof(float)*N);
	if ( fade != CONSTANT ) {
		float factor = fade == FADE_OUT ? 1.0f : 0.0f;
		float df = (fade == FADE_OUT ? -1.0f : 1.0f) /
			(float)sound_file->sample_length;
		for ( unsigned int i = 0;
			i < sound_file->sample_length;
			++ i ) {
				b[i] *= factor;
				factor += df;
		}
	}
	fftwf_complex* B = (fftwf_complex *) fftwf_malloc (
		sizeof (fftwf_complex) * MNh);
	memset(B,0,sizeof (fftwf_complex) * MNh);
	fftwf_plan fft_plan2 = fftwf_plan_dft_r2c_1d(
		MN,b,B,FFTW_ESTIMATE);	
	fftwf_execute(fft_plan2);
	fftwf_free(b);

	float scale = 1.0f / (float)MN;
	for ( unsigned int i = 0; i < MNh; ++ i ) {
		float re = (A[i][0] * B[i][0] - A[i][1] * B[i][1]) * scale;
		float im = (A[i][0] * B[i][1] + A[i][1] * B[i][0]) * scale;
		A[i][0] = re;
		A[i][1] = im;
	}

	fftwf_free(B);

	float* c = (float*) fftwf_malloc(sizeof (float) * MN);
	memset(c,0,sizeof(float)*MN);
	fftwf_plan inv_fft_plan = fftwf_plan_dft_c2r_1d(
		MN,A,c,FFTW_ESTIMATE);

	fftwf_execute(inv_fft_plan);

	fftwf_free(A);

	RecorderTrack* result = new RecorderTrack();
	RecorderTrack& _result = *result;

	for ( unsigned int i = 0; i < MN; ++ i ) {
		_result[i+sound_file->offset] = c[i];
	}

	fftwf_free(c);

	fftwf_destroy_plan (fft_plan1);
	fftwf_destroy_plan (fft_plan2);
	fftwf_destroy_plan (inv_fft_plan);	

	return result;
}

// Processes a sound file to include the response in the recorder
// track. The response is interpolated with another response to
// suggest the perception of movement from one location to the other.
// A Fast Fourier Transform is used to transfer both the dry signal
// and the impulse response to the frequency domain to reduce the
// complexity (= speed up) of the convolution operation.
// Contrary to the direct convolution mode, the FFT convolution mode
// does not allow the impulse response to be linearly interpolated
// hence the dry signal is faded in and out respectively before
// convolution occurs with the impulse response.
RecorderTrack* RecorderTrack::Process(RecorderTrack* const other,
                                      SoundFile* const sound_file)
                                      const {
	RecorderTrack* a = this->Process(sound_file, FADE_OUT);
	RecorderTrack* b = other->Process(sound_file, FADE_IN);

	a->Add(b);

	delete b;
	return a;
}
#else
// Processes a sound file to include the response in the recorder
// track. The response is not interpolated with a successive response
RecorderTrack* RecorderTrack::Process(SoundFile* const sound_file)
                                      const {
	RecorderTrack* result = new RecorderTrack();
	RecorderTrack& _result = *result;
	const RecorderTrack& _this = *this;
	const unsigned int len = this->real_length;
	const unsigned int sound_length = sound_file->sample_length;
	for ( unsigned int i = 0; i < sound_length; ++ i ) {
		const float sfs = sound_file->data[i];
		int index = i + sound_file->offset + first_sample;
		for ( unsigned int j = first_sample; j < len; ++ j ) {
			_result[index++] += sfs * _this[j];
		}
		DrawProgressBar(i,sound_length);
	}
	return result;
}
// Processes a sound file to include the response in the recorder
// track. The response is interpolated with another response to
// suggest the perception of movement from one location to the other
RecorderTrack* RecorderTrack::Process(RecorderTrack* const other,
                                      SoundFile* const sound_file)
                                      const {
	RecorderTrack* result = new RecorderTrack();
	RecorderTrack& _result = *result;
	const RecorderTrack& _this = *this;
	const RecorderTrack& _other = *other;
	const unsigned int sound_length = sound_file->sample_length;
	const float flt_samples = 1.0f / (float) sound_length;
	const unsigned int len = (std::max)(
		real_length,other->real_length);
	const unsigned int first = (std::min)(
		first_sample,other->first_sample);
	for( unsigned int i = 0; i < sound_length; i ++ ) {
		const float i1 = i * flt_samples;
		const float i2 = 1.0f - i1;
		const float sfs = sound_file->data[i];
		int index = i + sound_file->offset + first;
		for ( unsigned int j = first; j < len; j ++ ) {
			const float p = i2 * _this[j] + i1 * _other[j];
			_result[index++] += sfs * p;
		}
		DrawProgressBar(i,sound_length);
	}
	return result;
}
#endif
void RecorderTrack::Add(const RecorderTrack* other) {
	FloatBuffer& _this = *this;
	const RecorderTrack& _other = *other;
	const unsigned int len = other->getLength(0.0f);
	for ( unsigned int i = 0; i < len; ++ i ) {
		_this[i] += _other[i];
	}
}

float RecorderTrack::T60() const {
	const float attenuation_db = 60.0f;
	const float attenuation_gain = powf(10.0f,attenuation_db/20.0f);

	float direct_intensity = 0.0f;
	float min_gain = 0.0f;
	int last_significant_offset;
	int direct_sound_offset;

	const RecorderTrack& _this = *this;	

	float previous_sample = -1.0f;
	bool inside_indirect_lobe = false;
	for ( unsigned int j = first_sample; j < real_length; ++ j ) {
		const float sample = _this[j];
		if ( inside_indirect_lobe ) {
			if ( sample > min_gain ) {
				last_significant_offset = j;
			}
		} else if ( sample < previous_sample ) {
			// This is a rather silly way to determine the end of the
			// direct sound field, for it may not even been present
			// in this track and it is explicitely calculated
			// seperately from the reflections anyway in the
			// rendering function, but this information is no longer
			// available at this stage.
			inside_indirect_lobe = true;
			direct_intensity = previous_sample;
			min_gain = direct_intensity / attenuation_gain;
			direct_sound_offset = j;
		}
		previous_sample = sample;
	}

	const int reverberation_length = last_significant_offset -
		direct_sound_offset;
	return (float)reverberation_length/44100.0f;
}


void Recorder::Process(SoundFile* const sf, float offset) {
	for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
		SoundFile* section = sf->Section(offset);
		processed_tracks.push_back((*it)->Process(section));
		delete section;
	}
	is_processed = true;
}

void Recorder::Process(SoundFile* const sf, Recorder* r,
                       float offset, float length) {
	unsigned int track_id = 0;
	for ( TrackIt it = tracks.begin();
		it != tracks.end(); ++ it, ++ track_id ) {
		SoundFile* section = sf->Section(offset,length);
		processed_tracks.push_back(
			(*it)->Process(r->tracks[track_id],section));
		delete section;
	}
	is_processed = true;
}

void Recorder::Multiply(const float factor) {
	for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
		(*it)->Multiply(factor);
	}
}

void Recorder::Power(float a) {
	for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
		(*it)->Power(a);
	}
}

float Recorder::RootMeanSquare() {
	float acc = 0;
	for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
		const float track_rms = (*it)->RootMeanSquare();
		acc += track_rms * track_rms;
	}
	return sqrt(acc);
}

void Recorder::Truncate(Recorder* r2, Recorder* r3) {
	const float rms1 = this->RootMeanSquare() / 10000.0f;
	const float rms2 =   r2->RootMeanSquare() / 10000.0f;
	const float rms3 =   r3->RootMeanSquare() / 10000.0f;
	const int l1 = this->getLength(rms1);
	const int l2 =   r2->getLength(rms2);
	const int l3 =   r3->getLength(rms3);
	const int l = (std::max)(l1,(std::max)(l2,l3));
	this->Truncate(l);
	r2->Truncate(l);
	r3->Truncate(l);
}

void Recorder::Truncate(int len) {
	is_truncated = true;
	for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
		(*it)->Truncate(len);
	}
}

const float* Recorder::getSamples(unsigned int channel) {
	if ( channel >= tracks.size() ) return 0;
	const RecorderTrack& track = *tracks[channel];
	return &track[0];
}

unsigned int Recorder::getLength(float tresh) {
	if ( ! is_processed && ! has_samples ) return 0;
	unsigned int length = 0;
	if ( is_processed ) {
		for ( TrackIt it = processed_tracks.begin();
			it != processed_tracks.end(); ++ it ) {
			const unsigned int track_length = (*it)->getLength();
			if ( track_length > length ) length = track_length;
		}
	} else {
		for ( TrackIt it = tracks.begin();
			it != tracks.end(); ++ it ) {
			const unsigned int track_length =
				(*it)->getLength(tresh);
			if ( track_length > length ) length = track_length;
		}
	}
	return length;
}

void Recorder::Add(Recorder* r) {
	if ( trackCount() != r->trackCount() ) throw;
	for ( unsigned int i = 0;
		i < r->processed_tracks.size(); ++ i ) {
			if ( i == processed_tracks.size() )
				processed_tracks.push_back(new RecorderTrack());
			processed_tracks[i]->Add(r->processed_tracks[i]);
	}
	is_processed = true;
}

void Recorder::Normalize(float M) {
	float max = -1e9;
	TrackIt begin,end;
	if ( save_processed ) {
		begin = processed_tracks.begin();
		end = processed_tracks.end();
	} else {
		begin = tracks.begin();
		end = tracks.end();
	}
	for ( TrackIt it = begin; it != end; ++ it ) {
		const float track_max = (*it)->Maximum();
		if ( track_max > max ) max = track_max;
	}
	for ( TrackIt it = begin; it != end; ++ it ) {
		(*it)->Normalize(M,max);
	}
}

Recorder::~Recorder() {
	for ( TrackIt it = processed_tracks.begin();
		it != processed_tracks.end(); ++ it ) {
			delete *it;
	}
	for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
		delete *it;
	}		
}
