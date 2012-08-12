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

/// Whether to use fftw to process the convolution in the frequency domain
// #define USE_FFTW

#ifdef USE_FFTW
#ifdef _MSC_VER
#define FFTW_DLL
#endif
#include <fftw3.h>
#endif

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
	void resizeArray(const unsigned int l);
public:
	unsigned int first_sample;
	unsigned int real_length;
	FloatBuffer();
	~FloatBuffer();
	float& operator[] (const unsigned int i);
	const float& operator[] (const unsigned int i) const;
	/// Returns the Root Mean Square (or quadratic mean) of the
	/// data in the array. Any leading or trailing zero's are not
	/// included in the calculation.
	float RootMeanSquare() const;
	/// Returns the maximum value in the array.
	float Maximum() const;
	/// Multiplies all data in the array by a constant factor
	void Multiply(float f);
	/// Normalizes the data in the array. The first parameter defines
	/// the resulting maximum value in the buffer. The second parameter
	/// defines the original value that gets mapped to the value in
	/// the first parameter.
	void Normalize(float M = 1.0f, float MAX = -1.0f);
	/// Truncates (or matches) the buffer to this length.
	void Truncate(unsigned int l);
	/// Raises the data in the buffer to the power specified in a. The
	/// default of 0.67 is attributed to Stevens' power law:
	/// http://en.wikipedia.org/wiki/Stevens%27_power_law
	void Power(float a = 0.67f);
	/// Returns the length of the buffer incorporating a treshold
	/// that signals values under this treshold to be neglected.
	unsigned int getLength(float tresh = -1.0f) const;
	void Write(const std::string& fn) const;
	void Read(const std::string& fn);
};

/// This class represents a single impulse response of a listener. The main
/// use of this class is to provide a way to convolute sound files with this
/// impulse response.
class RecorderTrack : public FloatBuffer {
public:
#ifdef USE_FFTW
	typedef enum { CONSTANT, FADE_IN, FADE_OUT } Fade;
	/// Processes a sound file to include the response in the recorder track.
	/// The response is not interpolated with a successive response.
	/// A Fast Fourier Transform is used to transfer both the dry signal
	/// and the impulse response to the frequency domain to reduce the
	/// complexity (= speed up) of the convolution operation.
	/// An additional argument specifies whether to optionally fade in
	/// or fade out the input signal to help with the interpolation
	/// of successive key-frames.
	RecorderTrack* Process(SoundFile* const sound_file, Fade fade= CONSTANT ) const;
	/// Processes a sound file to include the response in the recorder track.
	/// The response is interpolated with another response to suggest the
	/// perception of movement from one location to the other.
	/// A Fast Fourier Transform is used to transfer both the dry signal
	/// and the impulse response to the frequency domain to reduce the
	/// complexity (= speed up) of the convolution operation.
	/// Contrary to the direct convolution mode, the FFT convolution mode
	/// does not allow the impulse response to be linearly interpolated
	/// hence the dry signal is faded in and out respectively before
	/// convolution occurs with the impulse response.
	RecorderTrack* Process(RecorderTrack* const other, SoundFile* const sound_file) const;
#else
	/// Processes a sound file to include the response in the recorder track.
	/// The response is not interpolated with a successive response.
	RecorderTrack* Process(SoundFile* const sound_file) const;
	/// Processes a sound file to include the response in the recorder track.
	/// The response is interpolated with another response to suggest the
	/// perception of movement from one location to the other.
	RecorderTrack* Process(RecorderTrack* const other, SoundFile* const sound_file) const;
#endif
	/// Linearly adds the data from the other recorder track to this one.
	void Add(const RecorderTrack* other);
	/// Returns the T60 reverberation time for the samples stored in this recorder track.
	/// From http://en.wikipedia.org/wiki/Reverberation
	/// T60 is the time required for reflections of a direct sound to decay by 60 dB below
	/// the level of the direct sound.
	float T60() const;
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
	void Process(SoundFile* const sf, float offset = 0.0f);
	/// Processes a sound file to include the responses in the tracks of the recorder.
	/// The responses are interpolated with another recorder to suggest the
	/// perception of movement from one location to the other.
	void Process(SoundFile* const sf, Recorder* r, float offset, float length);
	/// Multiplies all tracks in the recorder by a constant factor
	void Multiply(const float factor);
	/// Raises the tracks in the recorder to the power specified in a. The
	/// default of 0.67 is attributed to Stevens' power law:
	/// http://en.wikipedia.org/wiki/Stevens%27_power_law
	void Power(float a = 0.67f);
	/// Returns the Root Mean Square (or quadratic mean) of the recorder tracks.
	float RootMeanSquare();
	/// Matches the length of the tracks in this recorder and two other recorders.
	/// To prevent the processing of long tails that do not contribute to the
	/// final outcome due to their low intensity, a treshold value is calculated
	/// based of the Root Mean Square of the tracks, after which values are
	/// discarded.
	void Truncate(Recorder* r2, Recorder* r3);
	/// Truncates (or matches) the tracks in the recorder to this length.
	void Truncate(int len);
	/// Returns a const float array of the one of the tracks in the recorder.
	/// Use getLength() to determine the length of the returned array.
	const float* getSamples(unsigned int channel = 0);
	/// Returns maximum length of all tracks in this recorder incorporating
	/// a treshold that signals values under this treshold to be neglected.
	unsigned int getLength(float tresh = -1.0f);
	/// Linearly adds the tracks from the other recorder to this one.
	void Add(Recorder* r);
	/// Normalizes the tracks in this recorder. The parameter defines
	/// the resulting maximum value in the buffers.
	void Normalize(float M = 1.0f);
	~Recorder();
};

#endif