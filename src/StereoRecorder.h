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

#ifndef STEREORECORDER_H
#define STEREORECORDER_H

#include <sstream>
#include <iostream>
#include <fstream>

#include <gmtl/Point.h>
#include <gmtl/Vec.h>

#include "Recorder.h"

/// This class represents a listener approximated by a single cartesian
/// point and a direction vector of the right ear. Both can be animated.
/// The ear vector is used to add directivity to the final impulse
/// responses by taking the dot product of the ear vector and the
/// ray direction of the sample. The interaural time and -intensity
/// differences are based on this dot product. The intensity difference
/// also depend on the frequency bands, because the human head blocks
/// high frequencies more than low frequencies.
class StereoRecorder : public Datatype, public Recorder {
private:
	gmtl::Point3f location;
	gmtl::Vec3f right_ear;
	Animated<gmtl::Point3f>* animation;
	Animated<gmtl::Vec3f>* right_ear_animation;
public:	
	std::string filename;	
	//gmtl::Point3f getLocation(float t);
	void setLocation(gmtl::Point3f& loc);
	std::string getFilename();
	void setFilename(const std::string& s);
	int trackCount();
	
	StereoRecorder(bool fromFile = true);
	Recorder* getBlankCopy(int secs = -1);
	bool Save(const std::string& fn, bool norm = true, float norm_max = 1.0f);
	bool Save();
	inline void _Sample(int i, float v, int channel);
	void Record(const gmtl::Vec3f& dir, float ampl, float t, float dist, int band, int kf);
	void setLocation(gmtl::Point3f p);
	bool isAnimated();
	const gmtl::Point3f& getLocation(int i=-1) const;
	const gmtl::Vec3f& getRightEar(int i=-1);
	float getSegmentLength(int i);
	std::string toString();
	Animated<gmtl::Point3f>* getAnimationData();
};

#endif