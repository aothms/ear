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

#ifndef MONORECORDER_H
#define MONORECORDER_H

#include <sstream>
#include <iostream>
#include <fstream>

#include "boost/thread/thread.hpp"

#include "../lib/wave/WaveFile.h"

#include "SoundFile.h"
#include "Recorder.h"

/// This class represents a listener approximated by a single cartesian
/// point. It therefore has no orientation and captures sound from every
/// direction.
class MonoRecorder : public Datatype, public Recorder {
private:
	static boost::mutex mutex;
public:
	gmtl::Point3f location;
	std::string filename;
	Animated<gmtl::Point3f>* animation;
	gmtl::Point3f getLocation(float t);
	void setLocation(gmtl::Point3f& loc);
	std::string getFilename();
	void setFilename(const std::string& s);
	int trackCount();
	
	MonoRecorder(bool fromFile = true);
	Recorder* getBlankCopy(int secs = -1);
	bool Save(const std::string& fn, bool norm = true, float norm_max = 1.0f);
	bool Save();
	inline void _Sample(int i, float v);
	void Record(const gmtl::Vec3f& dir, float ampl, float t, float dist, int band, int kf);
	void setLocation(gmtl::Point3f p);
	bool isAnimated();
	//gmtl::Point3f getLocation();
	const gmtl::Point3f& getLocation(int i = -1) const;
	float getSegmentLength(int i);
	std::string toString();
	Animated<gmtl::Point3f>* getAnimationData();
};

#endif