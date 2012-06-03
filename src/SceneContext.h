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

#include "Recorder.h"
#include "SoundFile.h"

/// This class holds all data that is needed to render an impulse response.
/// The class is executable and can therefore be used as a context for a
/// thread.
class SceneContext {
public:
	std::vector<Recorder*> recorders;
	Scene* scene;
	int band;
	int soundfile_id;
	int keyframe_id;
	int samples;
	float absorption;
	float dry_level;
	void assignRecorders(Scene* s) {
		for ( std::vector<Recorder*>::const_iterator it = s->listeners.begin(); it != s->listeners.end(); ++ it ) {
			recorders.push_back((*it)->getBlankCopy(4));
		}
	}
	SceneContext(Scene* scn,int b,int sf, int s, float ab, float dr, int kf=-1) :
			scene(scn), band(b), soundfile_id(sf), keyframe_id(kf),
			samples(s), absorption(ab), dry_level(dr) {
		assignRecorders(scn);
	}
	void operator()() {		
		scene->Render(band,soundfile_id,absorption,samples,dry_level,recorders,keyframe_id);
	}
	std::string toString() const {
		std::stringstream ss;
		ss << "s:" << soundfile_id << " b:" << band << " k:" << keyframe_id;
		return ss.str();
	}
};

/// This class holds all data that is needed to convolute a sound file by
/// an impulse response. The class is executable and can therefore be used
/// as a context for a thread.
class RecorderContext {
public:
	SoundFile* soundfile;
	Recorder* recorder1;
	Recorder* recorder2;
	float offset;
	float length;
	RecorderContext(SoundFile* s, Recorder* r1, float o = 0.0f, Recorder* r2 = 0, float l = 0.0f) :
	soundfile(s), recorder1(r1), offset(o), recorder2(r2), length(l) {}
	void operator()() {
		if ( recorder2 ) {
			recorder1->Process(soundfile,recorder2,offset,length);
		} else {
			recorder1->Process(soundfile,offset);
		}
	}

};