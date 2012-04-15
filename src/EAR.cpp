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

#define BANNER "  ______                      _____  \n |  ____|         /\\         |  __ \\ \n | |__           /  \\        | |__) | \n |  __|         / /\\ \\       |  _  / \n | |____       / ____ \\      | | \\ \\ \n |______| (_) /_/    \\_\\ (_) |_|  \\_\\"
#define PRODUCT "Evaluation of Acoustics using Ray-tracing"
#define VERSION "0.1.4b"
#define HEADER BANNER "\n " PRODUCT "\n version " VERSION

#include <iostream>
#include <sstream>
#include <stdio.h>

#include <gmtl/gmtl.h>
#include <gmtl/Matrix.h>
#include <gmtl/Vec.h>
#include <gmtl/Ray.h>
#include <gmtl/Tri.h>
#include <gmtl/Output.h>
#include <gmtl/Intersection.h>

#include <boost/thread/thread.hpp>

#include "../lib/wave/WaveFile.h"
#include "../lib/equalizer/Equalizer.h"

#include "Settings.h"
#include "Mesh.h"
#include "SoundFile.h"
#include "MonoRecorder.h"
#include "StereoRecorder.h"
#include "Recorder.h"
#include "HelperFunctions.h"
#include "Scene.h"
#include "Material.h"
#include "SceneContext.h"

using boost::thread;

int Render(std::string filename, float* calc_T60=0, float* T60_Sabine=0, float* T60_Eyring=0) {
	
	// Init RNG, scene, and file input
	gmtl::Math::seedRandom((unsigned int) time(0));
	Scene* scene = new Scene();
	bool valid_file = Datatype::SetInput(filename);
	if ( ! valid_file ) {
		std::cout << "Failed to read file" << std::endl;
		return 1;
	}

	// Read the settings from file
	Datatype* settings = Datatype::Scan("SET ");
	if ( settings ) {
		Settings::init(settings);
		delete settings;
	} else {
		std::cout << "No settings block found in file" << std::endl;
		return 1;
	}

	gmtl::Vec3f absorption = Settings::GetVec("absorption");
	float dry_level = Settings::GetFloat("drylevel");
#ifdef _DEBUG
	int num_samples = Settings::GetInt("samples") / 1000;
#else
	int num_samples = Settings::GetInt("samples") / 10;
#endif
	int max_threads = Settings::IsSet("maxthreads") ?
		Settings::GetInt("maxthreads") : -1;
		
	if ( max_threads < 1 ) max_threads = -1; 

	std::string debugdir;
	bool has_debugdir = false;

	// Read rest of input file
	while ( Datatype::input_length ) {
		std::string peak = Datatype::PeakId();
		if ( peak == "OUT1" ) {
			Recorder* r = new MonoRecorder();
			scene->addListener(r);
		}
		else if ( peak == "OUT2" ) {
			Recorder* r = new StereoRecorder();
			scene->addListener(r);
		}
		else if ( peak == "SSRC" ) scene->addSoundSource(new SoundFile());
		else if ( peak == "3SRC" ) scene->addSoundSource(new TripleBandSoundFile());
		else if ( peak == "MESH" ) scene->addMesh(new Mesh());
		else if ( peak == "MAT " ) scene->addMaterial(new Material());
		else if ( peak == "SET " ) {delete Datatype::Read();}
		else if ( peak == "VRSN" ) {delete Datatype::Read();}
		else if ( peak == "KEYS" ) {Keyframes::Init();}
		else if ( peak == "FREQ" ) {
			Datatype::Read(false);
			const float f1 = Datatype::ReadFloat();
			const float f2 = Datatype::ReadFloat();
			const float f3 = Datatype::ReadFloat();
			SoundFile::SetEqBands(f1,f2,f3);
		} else {
			std::cout << "Unknown block '" << peak << "'" << std::endl;
			Datatype::Read();
		}
	}

	const char* lomihi[] = {"low","mid","high"};

	if ( Settings::IsSet("debugdir") ) {
		has_debugdir = true;
		debugdir = Settings::GetString("debugdir") + DIR_SEPERATOR;

		// Save equalizer output for debugging purposes
		int sf_id = 0;
		for ( std::vector<AbstractSoundFile*>::const_iterator it = scene->sources.begin(); it != scene->sources.end(); ++ it ) {
			for ( int band_id = 0; band_id < 3; ++ band_id ) {
				// If we are only here to calculate the T60 reverberation time
				// we are only going to render the mid frequency range.
				if ( calc_T60 && band_id != 1 ) continue;
				SoundFile* band = (*it)->Band(band_id);
				WaveFile w;
				w.FromFloat(band->data,band->sample_length);
				std::stringstream ss;
				ss << debugdir << "sound-" << sf_id << ".band-" << band_id << lomihi[band_id] << ".wav";
				w.Save(ss.str().c_str());
				delete band;
			}
			// If we are only here to calculate the T60 reverberation time
			// we are only going to render the first sound file encountered.
			if ( calc_T60 ) break;
			sf_id ++;
		}
	}

	if ( scene->meshes.empty() ) {
		std::cout << std::endl << "Warning: no reflective geometry" << std::endl << std::endl;
		scene->addMesh(Mesh::Empty());
	}

	Keyframes* keys = Keyframes::Get();

	std::cout << "Rendering..." << std::endl;

	// Create impules responses for sounds x keyframes x bands
	std::vector<SceneContext> scs;
	for( unsigned int sound_id = 0; sound_id < scene->sources.size(); sound_id ++ ) {
		AbstractSoundFile* sf = scene->sources[sound_id];
		// This for loop iterates over all keyframes. If the scene contains a static
		// configuration and no keyframes are present, keyframe_id is assigned -1.
		for( int keyframe_id = keys?0:-1; keyframe_id < (int)(keys?keys->keys.size():0); keyframe_id ++ ) {
			for( int band_id = 0; band_id < 3; band_id ++ ) {
				// If we are only here to calculate the T60 reverberation time
				// we are only going to render the mid frequency range.
				if ( calc_T60 && band_id != 1 ) continue;
				const float absorption_factor = 1.0f-absorption[band_id];
				SceneContext s(scene,band_id,sound_id,num_samples,absorption_factor,dry_level,keyframe_id);
				scs.push_back(s);
			}
			// If we are only here to calculate the T60 reverberation time
			// we are only going to render the first keyframe.
			if ( calc_T60 ) break;
		}
		// If we are only here to calculate the T60 reverberation time
		// we are only going to render the first sound file encountered.
		if ( calc_T60 ) break;
	}

	if ( max_threads > 0 )
		SetProgressBarSegments((int)ceil((float)scs.size()/(float)max_threads));

	{std::vector<SceneContext>::const_iterator it = scs.begin();
	while( true ) {
		boost::thread_group group;
		for ( int i = 0; max_threads < 0 || i < max_threads; i ++ ) {
			group.create_thread(*it);
			*it ++;
			if ( it == scs.end() ) break;
		}
		group.join_all();
		if ( max_threads > 0 ) NextProgressBarSegment();
		if ( it == scs.end() ) break;
	}}

	// Calculate max response
	float max = 0.0f;
	for( std::vector<SceneContext>::const_iterator it = scs.begin(); it != scs.end(); ++it ) {
		for ( std::vector<Recorder*>::const_iterator rit = it->recorders.begin(); rit != it->recorders.end(); ++ rit ) {
			Recorder* r1 = *rit;
			r1->Power(0.335f);
			for ( int i = 0; i < r1->trackCount(); ++ i ) {
				const float m = r1->tracks[i]->Maximum();
				if ( m > max ) max = m;
			}
		}
	}

	const float treshold = max / 256.0f;

	for( std::vector<SceneContext>::iterator it = scs.begin(); it != scs.end(); ++it ) {
		int rec_id = 0;
		for ( std::vector<Recorder*>::const_iterator rit = it->recorders.begin(); rit != it->recorders.end(); ++ rit ) {
			Recorder* r1 = *rit;
			r1->Truncate(r1->getLength(treshold));
			if ( has_debugdir ) {
				std::stringstream ss;
				const int sf_id = it->soundfile_id;
				const int band_id = it->band;
				const int kf_id = it->keyframe_id;
				ss << debugdir << "response-" << rec_id << ".sound-" << sf_id << ".frame-" << std::setw(2) << std::setfill('0') << kf_id << ".band-" << band_id << lomihi[band_id] << ".wav";
				r1->Save(ss.str(),true,max);
			}
			rec_id ++;
		}
	}	

	const bool noprocess = Settings::IsSet("noprocessing") && Settings::GetBool("noprocessing");
	if ( noprocess || calc_T60 ) {
		std::cout << std::endl << "Not processing data" << std::endl;
		
		if ( calc_T60 ) {
			
			// If we are only here to calculate the T60 reverberation time the rendered result
			// does not need to be convoluted. Instead, the T60 is determined based on the
			// rendered impulse response, as well as by the two well-known formulas Sabine
			// and Norris-Eyring. These deal with the prediction of reverberation time on a
			// statistical level. For a 'conventional' setup, the T60 that is calculated from
			// the impulse response should not deviate too much from the statistical prediction.

			const SceneContext sc = *scs.begin();
			const Recorder* rec = *sc.recorders.begin();
			const RecorderTrack* track = *rec->tracks.begin();
			*calc_T60 = track->T60();

			const Mesh* mesh = scene->meshes[0];
			const float V = mesh->Volume();
			const float A = mesh->TotalAbsorption();
			const float S = mesh->Area();
			const float a = mesh->AverageAbsorption();
			const float m = absorption[1];

			// Sabine:
			//     0.1611 V
			// T = -------
			//        A
			//
			// Norris-Eyring:
			//     -0.1611 V 
			// T = ---------
			//     S ln(1-a)

			if ( T60_Sabine )
				*T60_Sabine = 0.1611f*V/A;
			if ( T60_Eyring )
				*T60_Eyring = -0.1611f*V/(S*log(1.0f-a));
		}

		Datatype::Dispose();
		Keyframes::Dispose();
		delete scene;
		return 0;
	}

	std::cout << std::endl << "Processing data..." << std::endl;
	

	// Multiply impulses responses by sound file
	std::vector<RecorderContext> rcs;
	for( std::vector<SceneContext>::iterator it = scs.begin(); it != scs.end(); it ++ ) {
		const SceneContext sc = *it;
		const int band = sc.band;
		AbstractSoundFile* sf = scene->sources[sc.soundfile_id];
		if ( keys ) {
			const float offset = keys->keys[sc.keyframe_id];
			const int lastkey = keys->keys.size() - 1;
			if ( sc.keyframe_id == lastkey ) {
				for ( std::vector<Recorder*>::const_iterator rit = sc.recorders.begin(); rit != sc.recorders.end(); ++ rit ) {
					Recorder* r1 = *rit;
					rcs.push_back(RecorderContext(sf->Band(band),r1,offset));
				}
			} else {
				const SceneContext sc2 = *(it+3);
				std::vector<Recorder*>::const_iterator rit2 = sc2.recorders.begin();
				for ( std::vector<Recorder*>::const_iterator rit = sc.recorders.begin(); rit != sc.recorders.end(); ++ rit ) {
					Recorder* r1 = *rit;
					Recorder* r2 = *rit2;
					const float length = keys->keys[sc.keyframe_id+1] - offset;
					rcs.push_back(RecorderContext(sf->Band(band),r1,offset,r2,length));
					++ rit2;
				}				
			}
		} else {
			for ( std::vector<Recorder*>::const_iterator rit = sc.recorders.begin(); rit != sc.recorders.end(); ++ rit ) {
				Recorder* r1 = *rit;
				rcs.push_back(RecorderContext(sf->Band(band),r1));
			}
		}

	}
	
	if ( max_threads > 0 )
		SetProgressBarSegments((int)ceil((float)rcs.size()/(float)max_threads));
		
	{std::vector<RecorderContext>::const_iterator it = rcs.begin();
	while( true ) {
		boost::thread_group group;
		for ( int i = 0; max_threads < 0 || i < max_threads; i ++ ) {
			group.create_thread(*it);
			*it ++;
			if ( it == rcs.end() ) break;
		}
		group.join_all();
		if ( max_threads > 0 ) NextProgressBarSegment();
		if ( it == rcs.end() ) break;
	}}

	std::cout << std::endl << "Merging result..." << std::endl;

	// Add buffers and save
	int rec_id = 0;
	for ( std::vector<Recorder*>::const_iterator it = scene->listeners.begin(); it != scene->listeners.end(); ++ it ) {
		Recorder* r0 = *it;
		Recorder* total = r0->getBlankCopy();
		int scene_id = 0;
		for( std::vector<SceneContext>::iterator it = scs.begin(); it != scs.end(); it ++ ) {
			SceneContext sc = *it;
			Recorder* other = sc.recorders[rec_id];
			other->save_processed = true;
			if ( has_debugdir ) {
				std::stringstream ss;
				ss << debugdir << "rec-" << rec_id << ".sound-" << sc.soundfile_id << ".frame-" << std::setw(2) << std::setfill('0') << sc.keyframe_id << ".band-" << sc.band << ".wav";
				other->Save(ss.str());
			}
			total->Add(other);
			scene_id ++;
		}
		total->save_processed = true;
		total->Normalize(0.8f);
		total->Truncate(total->getLength(1e-6f));
		total->Save();
		rec_id ++;
	}

	Datatype::Dispose();
	Keyframes::Dispose();
	delete scene;

	return 0;
}

int main(int argc, char** argv) {
	std::cout << HEADER << std::endl << std::endl << std::endl;
	std::cout << std::setprecision(3) << std::fixed;
	for ( int i = 0; i < argc; i ++ ) {
		const std::string cmd(argv[i]);
		const std::string arg1 = ((i+1)<argc) ? std::string(argv[i+1]) : "";
		const std::string arg2 = ((i+2)<argc) ? std::string(argv[i+2]) : "";
		if ( cmd == "render" && !arg1.empty() ) {
			int ret_value = 1;
			try {
				ret_value = Render(arg1);
			} catch ( std::exception& e ) {
				std::cout << std::endl << "Error: " << e.what() << std::endl << std::endl;
			}
			std::cout << "Press a key to exit..." << std::endl;
			std::cin.get();
			return ret_value;
		} else if ( cmd == "calc" && arg1 == "T60" && !arg2.empty() ) {
			float T60_value, T60_sabine, T60_eyring;
			int ret_value = 1;
			try {
				ret_value = Render(arg2,&T60_value,&T60_sabine,&T60_eyring);
				std::cout << "T60_ear   : " << std::setprecision(9) << std::fixed << T60_value << "s" << std::endl;
				std::cout << "T60_sabine: " << std::setprecision(9) << std::fixed << T60_sabine << "s" << std::endl;
				std::cout << "T60_eyring: " << std::setprecision(9) << std::fixed << T60_eyring << "s" << std::endl;
			} catch ( std::exception& e ) {
				std::cout << std::endl << "Error: " << e.what() << std::endl << std::endl;				
			}
			return ret_value;
		}
	}
	std::cout << "Usage:" << std::endl 
		<< " EAR render <filename>" << std::endl
		<< " EAR calc T60 <filename>" << std::endl;
}

boost::mutex MonoRecorder::mutex;

