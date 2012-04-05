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

#ifndef ANIMATED_H
#define ANIMATED_H

#include <vector>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "Datatype.h"

/// This class represents the time offsets at which keyframes are placed.
/// Contrary to convention, keyframes times can not be individually specified
/// for different listeners or sources, but rather, their time offsets
/// are defined on a per-file basis, dictating the keyframe time coordinates
/// for all animated entities in the file.
class Keyframes: public Datatype {
private:
	static Keyframes* p;
public:
	std::vector<float> keys;
	Keyframes() {
		Read(false);
		assertid("KEYS");
		int l = *length;
		while( l ) {
			const float f = Datatype::ReadFloat();
			keys.push_back(f);
			// 2 * 4 bytes are read for the datatype
			// |flt4|xxxx|
			l -= 2 * 4;
		}
	}
	static void Init() {
		p = new Keyframes();
	}
	static void Dispose() {
		delete p;
	}
	static Keyframes* Get() {
		return p;
	}
};

/// This class represents the movements of listeners or sound sources if
/// they are set to be animated. The number of frames need to be equal to
/// the number of keyframe time offsets as read by the Keyframes class.
template <typename T>
class Animated : public Datatype {
public:	
	std::vector<T> frames;

	Animated() {
		Read(false);
		assertid("anim");
		int l = *length;
		Keyframes* keys = Keyframes::Get();
		if ( !keys || keys->keys.empty() ) {
			throw DatatypeException("Keyframe data not read");
		}
		while( l ) {
			frames.push_back(Datatype::ReadTriplet<T>());
			// 7 * 4 bytes are read for the datatype
			// |vec3|flt4|xxxx|flt4|xxxx|flt4|xxxx|
			l -= 7 * 4; 
		}
		if ( frames.size() != keys->keys.size() ) {
			throw DatatypeException("Keyframe count does not match");
		}
	}
	const T& Evaluate(int i){
		return frames[i];
	}
	float SegmentLength(int i){
		Keyframes* keys = Keyframes::Get();
		if ( i + 1 < (int)keys->keys.size() ) {
			return keys->keys[i+1] - keys->keys[1];
		} else {
			return -1.0f;
		}
	}
	std::string toString(){
		const T& f = frames.front();
		const T& b = frames.back();
		std::stringstream ss;
		ss << std::setprecision(std::cout.precision()) << std::fixed;
		ss << "< Animated (" << f.mData[0] << ", " << f.mData[1] << ", " << f.mData[2] << ") -> ("  << b.mData[0] << ", " << b.mData[1] << ", "  << b.mData[2] << ") >";
		return ss.str();
	}
};

#endif