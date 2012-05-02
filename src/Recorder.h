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

#ifndef RECORDER_H
#define RECORDER_H

#include <vector>
#include <stdio.h>
#include <string.h>
#include <algorithm>

#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>
#include <gmtl/Point.h>

#include "Animated.h"
#include "SoundFile.h"

#define SAMPLE_RATE (44100)
#define INITIAL_BUFFER_SIZE (3*SAMPLE_RATE)
#define INCREMENTAL_BUFFER_SIZE (1*SAMPLE_RATE)

/// This class behaves as a dynamic array of floating point numbers.
/// NOTE: The behaviour of this class differs whether it is a constant
/// or non-constant copy. In case of a non-constant instance, the 
/// element access operater automatically resizes the array in case
/// of accessing an element outside of the array bounds, whereas a constant
/// instance in that case would simply return 0.0.
class FloatBuffer {
private:
	float zero;
	float* data;
	unsigned int length;
	void resizeArray(const int l) {
		const float* old_data = data;
		data = new float[l];
		memcpy(data,old_data,length*sizeof(data[0]));
		memset(data+length,0,(l-length)*sizeof(data[0]));
		length = l;
		delete[] old_data;
	}
public:
	unsigned int first_sample;
	unsigned int real_length;
	FloatBuffer() {
		data = 0;
		real_length = length = 0;
		first_sample = INITIAL_BUFFER_SIZE -1;
		this->resizeArray(INITIAL_BUFFER_SIZE);
	}
	~FloatBuffer() {
		delete data;
	}
	float& operator[] (const unsigned int i) {
		if ( i >= length ) {
			resizeArray(i+INCREMENTAL_BUFFER_SIZE);
		}
		if ( i > real_length ) real_length = i;
		if ( i < first_sample ) first_sample = i;
		return data[i];
	}
	const float& operator[] (const unsigned int i) const {
		if ( i >= length ) {
			return zero;
		}
		return data[i];
	}
	/// Returns the Root Mean Square (or quadratic mean) of the
	/// data in the array. Any leading or trailing zero's are not
	/// included in the calculation.
	float RootMeanSquare() const {
		if ( ! real_length ) return 0;
		float x = 0.0;
		for ( unsigned int i = first_sample; i < real_length; i ++ ) {
			x += data[i]*data[i];
		}
		return sqrt(x/(float)real_length);
	}
	/// Returns the maximum value in the array.
	float Maximum() const {
		float x = 0.0;
		for ( unsigned int i = first_sample; i < real_length; i ++ ) {
			const float a = fabs(data[i]);
			if ( a > x ) x = a;
		}
		return x;
	}
	/// Multiplies all data in the array by a constant factor
	void Multiply(float f) {
		for ( unsigned int i = first_sample; i < real_length; i ++ ) {
			data[i] *= f;
		}
	}
	/// Normalizes the data in the array. The first parameter defines
	/// the resulting maximum value in the buffer. The second parameter
	/// defines the original value that gets mapped to the value in
	/// the first parameter.
	void Normalize(float M = 1.0f, float MAX = -1.0f) {
		Multiply( M / (MAX < 0 ? Maximum() : MAX) );
	}
	/// Truncates (or matches) the buffer to this length.
	void Truncate(unsigned int l) {
		if ( l == 0 ) l = 1;
		if ( l >= length ) {
			resizeArray(l);
		}
		real_length = l;
	}
	/// Raises the data in the buffer to the power specified in a. The
	/// default of 0.67 is attributed to Stevens' power law:
	/// http://en.wikipedia.org/wiki/Stevens%27_power_law
	void Power(float a = 0.67f) {
		for ( unsigned int i = first_sample; i < real_length; i ++ ) {
			data[i] = pow(data[i],a);
		}
	}
	/// Returns the length of the buffer incorporating a treshold
	/// that signals values under this treshold to be neglected.
	unsigned int getLength(float tresh = -1.0f) const {
		if ( tresh < 0.0f )
			return real_length;
		unsigned int max = 0;
		for ( unsigned int i = first_sample; i < length; ++ i ) {
			if ( data[i] >= tresh ) max = i;
		}
		return max + 1;
	}
};

/// This class represents a single impulse response of a listener. The main
/// use of this class is to provide a way to convolute sound files with this
/// impulse response.
class RecorderTrack : public FloatBuffer {
public:
	/// Processes a sound file to include the response in the recorder track.
	/// The response is not interpolated with a successive response.
	RecorderTrack* Process(SoundFile* const sound_file) const {
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
	/// Processes a sound file to include the response in the recorder track.
	/// The response is interpolated with another response to suggest the
	/// perception of movement from one location to the other.
	RecorderTrack* Process(RecorderTrack* const other, SoundFile* const sound_file) const {
		RecorderTrack* result = new RecorderTrack();
		RecorderTrack& _result = *result;
		const RecorderTrack& _this = *this;
		const RecorderTrack& _other = *other;
		const unsigned int sound_length = sound_file->sample_length;
		const float flt_samples = 1.0f / (float) sound_length;
		const unsigned int len = (std::max)(real_length,other->real_length);
		const unsigned int first = (std::min)(first_sample,other->first_sample);
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
	/// Linearly adds the data from the other recorder track to this one.
	void Add(const RecorderTrack* other) {
		FloatBuffer& _this = *this;
		const RecorderTrack& _other = *other;
		const unsigned int len = other->getLength(0.0f);
		for ( unsigned int i = 0; i < len; ++ i ) {
			_this[i] += _other[i];
		}
	}
	/// Returns the T60 reverberation time for the samples stored in this recorder track.
	/// From http://en.wikipedia.org/wiki/Reverberation
	/// T60 is the time required for reflections of a direct sound to decay by 60 dB below 
	/// the level of the direct sound.
	float T60() const {
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
				// This is a rather silly way to determine the end of the direct sound
				// field, for it may not even been present in this track and it is 
				// explicitely calculated seperately from the reflections anyway in the
				// rendering function, but this information is no longer available at
				// this stage.
				inside_indirect_lobe = true;
				direct_intensity = previous_sample;
				min_gain = direct_intensity / attenuation_gain;
				direct_sound_offset = j;
			}
			previous_sample = sample;
		}

		const int reverberation_length = last_significant_offset - direct_sound_offset;
		return (float)reverberation_length/44100.0f;
	}
	/// TODO: It would be great to implement other statistics as well. For example EDT
	/// (Early Decay Time) & STI (Speach Transmission Index). The StereoRecorder class
	/// could for example implement the IACC (Inter Aural Cross Correlation).
};

/// This class is the abstract base class for all classes of listeners. It defines
/// methods to record rendered samples and to use the data in the recorder for
/// convoluting sound files to include the rendered response in the final result.
class Recorder {
public:
	bool save_processed;
	bool is_processed;
	bool is_truncated;
	bool has_samples;
	int stamped_offset;
	typedef std::vector<RecorderTrack*> Tracks;
	typedef std::vector<RecorderTrack*>::const_iterator TrackIt; 
	Tracks tracks;
	Tracks processed_tracks;

	/// Returns the number of tracks in the recorder. E.g. 1 for mono, 2 for stereo.
	virtual int trackCount() = 0;
	/// Returns the location of the recorder for a certain keyframe index.
	virtual const gmtl::Point3f& getLocation(int i = -1) const = 0;
	/// Sets the constant location of the listener.
	virtual void setLocation(gmtl::Point3f& loc) = 0;
	/// Returns the filename of to where the final result will be written.
	virtual std::string getFilename() = 0;
	/// Gets a blank copy of a recorder with the same amount of tracks.
	virtual Recorder* getBlankCopy(int secs = -1) = 0;
	/// Returns the animated location of the recorder in case it is defined.
	virtual Animated<gmtl::Point3f>* getAnimationData() = 0;
	/// Returns whether the recorder is animated.
	virtual bool isAnimated() = 0;
	/// Records a sample to one or all of the recorder tracks. The direction of the sample
	/// can be used to simulate stereo recording or use Head Related Transfer Functions.
	/// The amplitude is the intensity of the sample. Time is the total path length of the
	/// sample divided by the speed of sound. The distance is used to splat the sample over
	/// the buffer using a filter. The band is used to incorporate properties that differ per
	/// frequency, such as the filter or potentially the HRTF.
	virtual void Record(const gmtl::Vec3f& dir, float ampl, float t, float dist, int band, int kf) = 0;
	/// Saves the data in the recorder to the specified filename. In case the the recorder contains
	/// processed data, the member save_processed dictates whether the convoluted sound file
	/// or the raw impulse resonse is written to file.
	virtual bool Save(const std::string& s, bool norm = true, float norm_max = -1.0f) = 0;
	/// Saves the data in the recorder to the specified filename. In case the the recorder contains
	/// processed data, the member save_processed dictates whether the convoluted sound file
	/// or the raw impulse resonse is written to file.
	virtual bool Save() = 0;
	/// Processes a sound file to include the responses in the tracks of the recorder.
	/// The responses are not interpolated with a successive responses.
	void Process(SoundFile* const sf, float offset = 0.0f) {
		for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
			SoundFile* section = sf->Section(offset);
			processed_tracks.push_back((*it)->Process(section));
			delete section;
		}
		is_processed = true;
	}
	/// Processes a sound file to include the responses in the tracks of the recorder.
	/// The responses are interpolated with another recorder to suggest the
	/// perception of movement from one location to the other.
	void Process(SoundFile* const sf, Recorder* r, float offset, float length) {
		unsigned int track_id = 0;
		for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it, ++ track_id ) {
			SoundFile* section = sf->Section(offset,length);
			processed_tracks.push_back((*it)->Process(r->tracks[track_id],section));
			delete section;
		}
		is_processed = true;
	}
	/// Multiplies all tracks in the recorder by a constant factor
	void Multiply(const float factor) {
		for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
			(*it)->Multiply(factor);
		}
	}
	/// Raises the tracks in the recorder to the power specified in a. The
	/// default of 0.67 is attributed to Stevens' power law:
	/// http://en.wikipedia.org/wiki/Stevens%27_power_law
	void Power(float a = 0.67f) {
		for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
			(*it)->Power(a);
		}
	}
	/// Returns the Root Mean Square (or quadratic mean) of the recorder tracks.
	float RootMeanSquare() {
		float acc = 0;
		for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
			const float track_rms = (*it)->RootMeanSquare();
			acc += track_rms * track_rms;
		}
		return sqrt(acc);
	}
	/// Matches the length of the tracks in this recorder and two other recorders.
	/// To prevent the processing of long tails that do not contribute to the
	/// final outcome due to their low intensity, a treshold value is calculated
	/// based of the Root Mean Square of the tracks, after which values are
	/// discarded.
	void Truncate(Recorder* r2, Recorder* r3) {
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
	/// Truncates (or matches) the tracks in the recorder to this length.
	void Truncate(int len) {
		is_truncated = true;
		for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
			(*it)->Truncate(len);
		}
	}
	/// Returns a const float array of the one of the tracks in the recorder.
	/// Use getLength() to determine the length of the returned array.
	const float* getSamples(unsigned int channel = 0) {
		if ( channel >= tracks.size() ) return 0;
		const RecorderTrack& track = *tracks[channel];
		return &track[0];
	}
	/// Returns maximum length of all tracks in this recorder incorporating 
	/// a treshold that signals values under this treshold to be neglected.
	unsigned int getLength(float tresh = -1.0f) {
		if ( ! is_processed && ! has_samples ) return 0;
		unsigned int length = 0;
		if ( is_processed ) {
			for ( TrackIt it = processed_tracks.begin(); it != processed_tracks.end(); ++ it ) {
				const unsigned int track_length = (*it)->getLength();
				if ( track_length > length ) length = track_length;
			}
		} else {
			for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
				const unsigned int track_length = (*it)->getLength(tresh);
				if ( track_length > length ) length = track_length;
			}
		}
		return length;
	}
	/// Linearly adds the tracks from the other recorder to this one.
	void Add(Recorder* r) {
		if ( trackCount() != r->trackCount() ) throw;
		for ( unsigned int i = 0; i < r->processed_tracks.size(); ++ i ) {
			if ( i == processed_tracks.size() ) processed_tracks.push_back(new RecorderTrack());
			processed_tracks[i]->Add(r->processed_tracks[i]);
		}
		is_processed = true;
	}
	/// Normalizes the tracks in this recorder. The parameter defines
	/// the resulting maximum value in the buffers.
	void Normalize(float M = 1.0f) {
		float max = -1e9;
		for ( TrackIt it = (save_processed ? processed_tracks.begin() : tracks.begin()); it != (save_processed ? processed_tracks.end() : tracks.end()); ++ it ) {
			const float track_max = (*it)->Maximum();
			if ( track_max > max ) max = track_max;
		}
		for ( TrackIt it = (save_processed ? processed_tracks.begin() : tracks.begin()); it != (save_processed ? processed_tracks.end() : tracks.end()); ++ it ) {
			(*it)->Normalize(M,max);
		}
	}
	~Recorder() {
		for ( TrackIt it = processed_tracks.begin(); it != processed_tracks.end(); ++ it ) {
			delete *it;
		}
		for ( TrackIt it = tracks.begin(); it != tracks.end(); ++ it ) {
			delete *it;
		}		
	}
};

#endif