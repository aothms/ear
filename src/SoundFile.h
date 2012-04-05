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

#ifndef SOUNDFILE_H
#define SOUNDFILE_H

#include <gmtl/Point.h> 

#include "../lib/wave/WaveFile.h"
#include "../lib/equalizer/Equalizer.h"

#include "Mesh.h"
#include "Animated.h"
#include "HelperFunctions.h"

class SoundFile;

/// This is the abstract base class of all sound files. It outlines methods
/// related to the location/animation of the sound source, the sample data
/// of the wave file and functionality to generate a ray form the origin point
/// or mesh if the latter is defined.
class AbstractSoundFile : public Datatype {
protected:
	Mesh* mesh;
	float gain;
	bool sample_owner;
public:
	float* data;
	unsigned int sample_length;
	unsigned int offset;
	
	gmtl::Point3f location;
	
	Animated<gmtl::Point3f>* animation;
	void setLocation(gmtl::Point3f p);
	bool isAnimated();
	gmtl::Point3f getLocation();
	gmtl::Point3f getLocation(int i);
	
	gmtl::Rayf* SoundRay(int keyframeID = -1);
	bool isMeshSource();
	float getGain();

	/// Returns only the corresponding frequency band of the file. Regardless of
	/// the exact instantiation class, it always returns a SoundFile*.
	virtual SoundFile* Band(int I) = 0;
	virtual std::string toString() = 0;
	virtual ~AbstractSoundFile() {};
};

/// This class inherits from AbstractSoundFile and instantiates a sound source
/// based on a single source .WAVE file. To split the file into the three
/// frequency bands an equalizer algorithm is used.
class SoundFile : public AbstractSoundFile {
private:
	std::string filename;
public:
	static float f1;
	static float f2;
	static float f3;
	SoundFile();
	~SoundFile();
	SoundFile(float* data,int length, unsigned int offset, bool is_owner);
	SoundFile* Band(int I);
	/// Returns a section of the sound file.
	/// NOTE: No data is copied on the pointer to the first element is increased.
	SoundFile* Section(unsigned int start, unsigned int length);
	/// Returns a section of the sound file.
	/// NOTE: No data is copied on the pointer to the first element is increased.
	SoundFile* Section(float start, float length = -1.0f);
	std::string toString();
	/// Sets the center frequencies of the three frequency bands used by the
	/// equalizer algorithm.
	static void SetEqBands(float f1,float f2,float f3);
};

/// This class inherits from AbstractSoundFile and instantiates a sound source
/// based on three source .WAVE file. Therefore the equalizer algorithm
/// does not need to be used.
class TripleBandSoundFile : public AbstractSoundFile {
private:
	SoundFile* soundfiles[3];
	std::string filename[3];
public:
	TripleBandSoundFile();
	~TripleBandSoundFile();
	SoundFile* Band(int I);
	std::string toString();
};

#endif