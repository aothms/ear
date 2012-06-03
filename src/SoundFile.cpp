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

#include <algorithm>
#include <string.h>

#include <gmtl/Point.h>

#include "../lib/wave/WaveFile.h"
#include "../lib/equalizer/Equalizer.h"

#include "Animated.h"
#include "HelperFunctions.h"
#include "SoundFile.h"
#include "Distributions.h"

SoundFile::SoundFile() {
	Read(false);
	assertid("SSRC");
	filename = ReadString();
	WaveFile w(filename.c_str());
	data = w.ToFloat();
	sample_owner = true;
	sample_length = w.GetSampleSize();
	if ( !data || !sample_length ) {
		throw DatatypeException("Failed to open sound file " + filename);
	}
	soundfiles[0] = soundfiles[1] = soundfiles[2] = 0;
	data_low = data_mid = data_high = 0;
	mesh = 0;
	animation = 0;
	if ( Datatype::PeakId() == "anim" ) {
		animation = new Animated<gmtl::Point3f>();
	} else if ( Datatype::PeakId() == "mesh" ) {
		Datatype::prefix = " +- " + Datatype::prefix;
		mesh = new Mesh();
		Datatype::prefix = Datatype::prefix.substr(4);
	} else {
		setLocation(ReadPoint());			
	}
	if ( Datatype::PeakId() == "flt4" ) {
		gain = Datatype::ReadFloat();
	} else {
		gain = 1.0f;
	}
	if ( Datatype::PeakId() == "flt4" ) {
		offset = (unsigned int) (Datatype::ReadFloat() * 44100.0f);
	} else {
		offset = 0;
	}
	std::cout << this->toString();
	std::cout << Datatype::stringblock;
	Datatype::stringblock = "";
}
SoundFile::~SoundFile() {
	if ( sample_owner ) {
		delete mesh;
		delete animation;
		delete[] data;
		delete[] data_low;
		delete[] data_mid;
		delete[] data_high;
		delete[] soundfiles[0];
		delete[] soundfiles[1];
		delete[] soundfiles[2];
	}
}
TripleBandSoundFile::TripleBandSoundFile() {
	Read(false);
	assertid("3SRC");
	for ( int i = 0; i < 3; ++ i ) {
		std::string fn = ReadString();
		WaveFile w(fn.c_str());
		soundfiles[i] = new SoundFile(w.ToFloat(),w.GetSampleSize(),0,true);
		filename[i] = fn;
	}	
	mesh = 0;
	animation = 0;
	if ( Datatype::PeakId() == "anim" ) {
		animation = new Animated<gmtl::Point3f>();
	} else if ( Datatype::PeakId() == "mesh" ) {
		Datatype::prefix = " +- " + Datatype::prefix;
		mesh = new Mesh();
		Datatype::prefix = Datatype::prefix.substr(4);
	} else {
		setLocation(ReadPoint());			
	}
	if ( Datatype::PeakId() == "flt4" ) {
		gain = Datatype::ReadFloat();
	} else {
		gain = 1.0f;
	}
	if ( Datatype::PeakId() == "flt4" ) {
		offset = (unsigned int) (Datatype::ReadFloat() * 44100.0f);
	} else {
		offset = 0;
	}
	std::cout << this->toString();
	std::cout << Datatype::stringblock;
	Datatype::stringblock = "";
}
TripleBandSoundFile::~TripleBandSoundFile() {
	for ( int i = 0; i < 3; ++ i ) {
		delete soundfiles[i];
	}
	delete animation;
	delete mesh;	
}
SoundFile::SoundFile(float* d,int length, unsigned int o, bool is_owner) {
	data = d;
	sample_owner = is_owner;
	sample_length = length;
	offset = o;
	mesh = 0;
	animation = 0;
}
void AbstractSoundFile::setLocation(gmtl::Point3f p) {
	location = p;
}
bool AbstractSoundFile::isAnimated() { return animation > 0; }
gmtl::Point3f AbstractSoundFile::getLocation() {
	return location;
}
gmtl::Point3f AbstractSoundFile::getLocation(int i) {
	if ( i >= 0 && animation > 0 ) {
		return animation->Evaluate(i);
	} else {
		return location;
	}
}
SoundFile* SoundFile::Band(int I) {
	if ( !soundfiles[0] ) {
		data_low = new float[sample_length];
		data_mid = new float[sample_length];
		data_high = new float[sample_length];
		Equalizer::Split(data,data_low,data_mid,data_high,sample_length,f1*1000.0f,f2*1000.0f,f3*1000.0f);
		soundfiles[0] = new SoundFile(data_low,sample_length,0,false);
		soundfiles[1] = new SoundFile(data_mid,sample_length,0,false);
		soundfiles[2] = new SoundFile(data_high,sample_length,0,false);
	}
	return soundfiles[I];
}
SoundFile* TripleBandSoundFile::Band(int I) {
	const SoundFile* sf = soundfiles[I];
	return new SoundFile(sf->data,sf->sample_length,offset,false);
}
SoundFile* SoundFile::Section(unsigned int start, unsigned int length) {
	if ( start >= sample_length )
		return new SoundFile(0,0,0,false);
	else return new SoundFile(data+start,std::min(length,sample_length-start),offset+start,false);
}
SoundFile* SoundFile::Section(float start, float length) {
	const unsigned int int_start = (int) (start * 44100.0f);
	const unsigned int int_length = length < 0 ? sample_length - int_start : (int) (length * 44100.0f);
	return Section(int_start,int_length);
}
std::string SoundFile::toString() {
	std::string loc;
	if ( ! mesh ) {
		loc = " +- location: ";
		if ( this->isAnimated() ) {
			loc += this->animation->toString();
		} else {
			std::stringstream ss;
			ss << std::setprecision(std::cout.precision()) << std::fixed;
			ss << this->location;
			loc += ss.str();
		}
		loc += "\r\n";
	}
	std::stringstream ss;
	ss << "Sound source\r\n" << loc << " +- data: " << FileName(filename) << " [" << sample_length << " samples]" << std::endl << " +- offset: " << offset << std::endl;
	return ss.str();
}
std::string TripleBandSoundFile::toString() {
	std::string loc;
	if ( ! mesh ) {
		loc = " +- location: ";
		if ( this->isAnimated() ) {
			loc += this->animation->toString();
		} else {
			std::stringstream ss;
			ss << std::setprecision(std::cout.precision()) << std::fixed;
			ss << this->location;
			loc += ss.str();
		}
		loc += "\r\n";
	}
	std::stringstream ss;
	ss << "Sound source\r\n" << loc;
	for ( int i = 0; i < 3; ++ i ) {
		ss << " +- data" << (i+1) << ": " << FileName(filename[i]) << " [" << soundfiles[i]->sample_length << " samples]" << std::endl;
	}
	ss << " +- offset: " << offset << std::endl;
	return ss.str();
}

gmtl::Rayf* AbstractSoundFile::SoundRay(int keyframeID) {
	if ( mesh > 0 ) {
		gmtl::Point3f p;
		gmtl::Vec3f n,d;
		mesh->SamplePoint(p,n);
		Sample_Hemi(d,n);
		return new gmtl::Rayf(p,d);			
	} else {
		gmtl::Point3f p = getLocation(keyframeID);
		gmtl::Vec3f d;
		Sample_Sphere(d);
		return new gmtl::Rayf(p,d);
	}
}

bool AbstractSoundFile::isMeshSource() {
	return mesh > 0;
}

void SoundFile::SetEqBands(float F1, float F2, float F3) {
	f1 = F1; f2 = F2; f3 = F3;
}

float AbstractSoundFile::getGain() { return gain; }

float SoundFile::f1 =  0.3f;
float SoundFile::f2 =  2.0f;
float SoundFile::f3 = 16.0f;