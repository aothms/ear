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

#include <sstream>
#include <iostream>
#include <fstream>

#include "boost/thread/thread.hpp"

#include "../lib/wave/WaveFile.h"

#include "SoundFile.h"
#include "MonoRecorder.h"

#define USE_FILTER

void MonoRecorder::setLocation(gmtl::Point3f& loc) { location = loc; }
std::string MonoRecorder::getFilename() { return filename; }
void MonoRecorder::setFilename(const std::string& s) { filename = s; }
int MonoRecorder::trackCount() { return 1; }

MonoRecorder::MonoRecorder(bool fromFile) {
	is_truncated = is_processed = false;
	if ( fromFile ) {
		stamped_offset = 0;
		Read(false);
		assertid("OUT1");
		filename = ReadString();
		float t = ReadFloat();
		if ( Datatype::PeakId() == "anim" ) {
			animation = new Animated<gmtl::Point3f>();
		} else {
			setLocation(ReadVec());
			animation = 0;
		}
		std::cout << this->toString();
	} else {
		animation = 0;
		filename = "";
	}
	has_samples = save_processed = false;
	tracks.push_back(new RecorderTrack());
}
Recorder* MonoRecorder::getBlankCopy(int secs) {
	MonoRecorder* r = new MonoRecorder(false);
	r->stamped_offset = 0;
	r->filename = filename;
	r->location = location;
	r->animation = animation;
	r->save_processed = save_processed;
	return r;
}
bool MonoRecorder::Save(const std::string& fn, bool norm, float norm_max) {
	WaveFile w;
	const RecorderTrack& to_save = *(save_processed ? processed_tracks[0] : tracks[0]);
	w.FromFloat(&to_save[0],to_save.getLength(),norm,norm_max);
	w.Save(fn.c_str());
	return true;
}
bool MonoRecorder::Save() {
	return Save(filename,false);
}
inline void MonoRecorder::_Sample(int i, float v) {
	this->tracks[0]->operator [](i) += v;
	has_samples = true;
}
void MonoRecorder::Record(const gmtl::Vec3f& dir, float a, float t, float dist, int band, int kf) {
	const int s = (int) (t*44100.0);
#ifdef USE_FILTER	
	const float width = sqrt(dist);
	float ampl = 2.0f * a / width;
	const int w = (int) ceil(width);
	const float step = ampl / w;
	for ( int i = 0; i < w; ++ i ) {
		_Sample(i+s,ampl);
		ampl -= step;
	}
#else
	_Sample(s,a);
#endif
}
void MonoRecorder::setLocation(gmtl::Point3f p) {
	location = p;
}
bool MonoRecorder::isAnimated() { return animation > 0; }
const gmtl::Point3f& MonoRecorder::getLocation(int i) const {
	if ( i >= 0 && animation > 0 ) {
		return animation->Evaluate(i);
	} else {
		return location;
	}
}
float MonoRecorder::getSegmentLength(int i) {
	return animation->SegmentLength(i);
}
std::string MonoRecorder::toString() {
	std::string loc;
	if ( this->isAnimated() ) {
		loc = this->animation->toString();
	} else {
		std::stringstream ss;
		ss << std::setprecision(std::cout.precision()) << std::fixed;
		ss << this->location;
		loc = ss.str();
	}
	return "Recorder\n +- mono\n +- location: " + loc + "\n";
}
Animated<gmtl::Point3f>* MonoRecorder::getAnimationData() {
	return animation;
}