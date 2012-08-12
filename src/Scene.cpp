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

#include <vector>
#include <map>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>
#include <gmtl/LineSeg.h>

#include "Material.h"
#include "MonoRecorder.h"
#include "Distributions.h"
#include "Scene.h"

// Contributions that are negative, zero, denormal, NaN or infinite are to be discarded
#ifdef _MSC_VER
#define INVALID_FLOAT(x) (_fpclass(x)==2)
#else
#include <cmath>
#define INVALID_FLOAT(x) (std::fpclassify(x)!=FP_NORMAL)
#endif

#define DO_PHASE_INVERSION

// The exponent for specular reflections
#define EXP 1000.0f
#define EXP_INT (EXP + 1.0f)

gmtl::Rayf* Scene::Bounce(int band, gmtl::Rayf* sound_ray,
						  gmtl::Vec3f*& surface_normal, float& l,
						  Material*& mat, BounceType& bt) {
	gmtl::Point3f* p = 0;
	gmtl::Rayf* old_sound_ray = 0;

	// Only testing intersections with meshes[0] because it
	// contains a combination of all meshes added to the scene
	if ( meshes[0]->RayIntersection(
		sound_ray,p,surface_normal,mat) ) {

		bt = mat->Bounce(band);
		gmtl::Vec3f v;

		const float spec = mat->specularity_coefficient[band];

		if ( bt == REFRACT ) {
			gmtl::Vec3f* n2 = new gmtl::Vec3f(-*surface_normal);
			delete surface_normal;
			surface_normal = n2;
			Sample_Hemi(v, *surface_normal, sound_ray->mDir, spec);
		} else {
			gmtl::Vec3f refl = gmtl::reflect(
				refl, sound_ray->mDir, *surface_normal);
			Sample_Hemi(v, *surface_normal, refl, spec);
		}

		old_sound_ray = new gmtl::Rayf(*p,v);
		const gmtl::Vec3f dist = *p - sound_ray->mOrigin;
		l = gmtl::length(dist);
	}
	delete p;
	return old_sound_ray;
}

gmtl::LineSegf* Scene::Connect(const gmtl::Point3f* p,
							   const gmtl::Point3f& x) {
	// Create a line segment and test for line-triangle intersections
	gmtl::LineSegf* ls = new gmtl::LineSegf(*p,x);
	// Only testing intersections with meshes[0] because it contains
	// a combination of all meshes added to the scene
	if ( meshes[0]->LineIntersection(ls) ) {
		delete ls;
		return 0;
	} else {
		return ls;
	}
}
void Scene::addListener(Recorder* l) {
	listeners.push_back(l);
}
void Scene::addSoundSource(AbstractSoundFile* s) {
	sources.push_back(s);
}
void Scene::addMesh(Mesh* m) {
	if ( meshes.empty() ) meshes.push_back(m);
	else meshes[0]->Combine(m);
}
void Scene::addMaterial(Material* m) {
	Mesh::materials[m->name] = m;
}

void Scene::Render(int band, int sound, float absorbtion_factor,
				   int num_samples, float dry,
				   const std::vector<Recorder*>& recs,
				   int keyframeID) {

	gmtl::Math::seedRandom((int)time(NULL));

	AbstractSoundFile* currentSound = sources[sound];
	const gmtl::Point3f sfloc =
		currentSound->getLocation(keyframeID);

	float amount = 0;

	for( int sample_count=0; sample_count < num_samples;
		sample_count ++ ) {

		amount += 1.0f;

		DrawProgressBar(sample_count,num_samples);

		float sample_intensity = 1.0;
		gmtl::Rayf* sound_ray = 0;
		gmtl::Rayf* old_sound_ray = 0;
		gmtl::Vec3f* surface_normal = 0;
		float total_path_length = 0.0f;

		gmtl::Vec3f prev_ray_dir = gmtl::Vec3f();

		float segment_length = 0.0f;
		Material* mat = 0;
		BounceType bt;

		for( int num_bounces = 0; num_bounces < 1000;
			num_bounces ++ ) {

			surface_normal = 0;
			mat = 0;
			if ( ! sound_ray ) {
				sound_ray = currentSound->SoundRay(keyframeID);
			} else {
				old_sound_ray = Bounce(band,sound_ray,
					surface_normal,segment_length,mat,bt);

				sample_intensity *= pow(absorbtion_factor,segment_length);

				total_path_length += segment_length;
				delete sound_ray;
				sound_ray = old_sound_ray;
			}

			const float spec_coef = (mat > 0)
				? mat->specularity_coefficient[band]
				: 0;

			// Failed to generate valid bounce, terminate path
			if ( sound_ray == 0 ) break;

			// Account for energy loss by absorbtion:
			if ( num_bounces > 0 ) {				
				sample_intensity *= mat->absorption_coefficient[band];
			}

			const float sample_intensity_before_bounce = sample_intensity;

			if ( INVALID_FLOAT(sample_intensity) ) {
				break;
			}

			// Direct sound is added in a separate step in the end,
			// because in future versions it might be stored
			// separately, for example to be able to reproduce
			// phenomena like Doppler effect. In case the sound
			// source emits from a mesh, the direct sound is
			// sampled regardless.
			if ( num_bounces || currentSound->isMeshSource() ) {

				// For every recorder in the scene..
				for ( std::vector<Recorder*>::const_iterator
					it = recs.begin(); it != recs.end(); ++ it ) {
					Recorder* rec = *it;

					// See if the intersection point of the ray is
					// 'visible' from the recorder location
					gmtl::LineSegf* ls = Connect(&sound_ray->mOrigin,
						rec->getLocation(keyframeID));

					if ( ls ) {

						// Because triangles in EAR are two-sided we might need
						// to re-orient the surface normal of the triangle based
						// on its dot product with the linesegment direction
						const gmtl::Vec3f lsdir = gmtl::makeNormal(ls->mDir);
						bool valid = true;

						const float dot = num_bounces
							? gmtl::dot(lsdir,*surface_normal)
							: 1.0f;

						if ( dot > 0 ) {
							float this_sample_intensity =
								sample_intensity_before_bounce;

							// A valid path from the intersection point to the
							// listener location has been found, now we need to
							// determine the intensity of the contribution of
							// the ray.
							if ( num_bounces) {

							if ( bt == REFLECT ) {
								gmtl::Vec3f refl_vector;
								gmtl::reflect(refl_vector,
									prev_ray_dir,*surface_normal);

								const float diff_factor =
									-gmtl::dot(*surface_normal,
									prev_ray_dir);

								const float spec_factor = (std::max)(0.0f,
									gmtl::dot(refl_vector,lsdir));

								const float factor = spec_coef *
									EXP_INT * pow(spec_factor,EXP) +
									(1.0f - spec_coef) * diff_factor;

								this_sample_intensity *= factor;
							} else {
								const float diff_factor = gmtl::dot(
									*surface_normal,prev_ray_dir);
								const float spec_factor = (std::max)(0.0f,
									gmtl::dot(prev_ray_dir,lsdir));

								const float factor = spec_coef *
									EXP_INT * pow(spec_factor,EXP) +
									(1.0f - spec_coef) * diff_factor;

								this_sample_intensity *= factor;
							}
							}

							const float l = ls->getLength();
							this_sample_intensity *= pow(absorbtion_factor,l);
							this_sample_intensity *= INV_HEMI_2(l);

							if ( !INVALID_FLOAT(
								this_sample_intensity) ) {
#ifdef DO_PHASE_INVERSION
									if ( num_bounces % 2 ) this_sample_intensity *= -1.0f;
#endif
									rec->Record(lsdir,this_sample_intensity,
										(total_path_length+l)/343.0f,
										total_path_length+l,band,keyframeID);
							}
						}
					}

					delete ls;
					ls = 0;
				}

			}

			// Arbitrary constant, ideally this would be determined
			// based on some heuristics or previously collected
			// samples.
			if ( sample_intensity < 0.00000001 ) break;

			prev_ray_dir = gmtl::makeNormal(sound_ray->mDir);

			delete surface_normal;
			surface_normal = 0;
		}
		delete surface_normal;
		delete sound_ray;
	}

	// For every recorder in the scene...
	std::vector<Recorder*>::const_iterator it = recs.begin();
	for ( ; it != recs.end(); ++ it ) {
		Recorder* rec = *it;

		rec->Multiply(1.0f / amount);

		// The direct sound lobe is added...
		// Ideally this lobe would be stored separately from the rest
		// of the samples, it could then be subject of dopler-effect
		// calculations for example and would ease the calculation of
		// some of the statistical properties of the rendered impulse
		// response.
		if ( !currentSound->isMeshSource() ) {

		const gmtl::Point3f listener_location =
			rec->getLocation(keyframeID);
		gmtl::LineSegf* ls = Connect(&listener_location,sfloc);
		if ( ls ) {
			const gmtl::Vec3f dist = listener_location - sfloc;
			const float len = gmtl::length(dist);
			const gmtl::Vec3f dir = gmtl::makeNormal(dist);
			rec->Record(dir,INV_SPHERE_2(len)*pow(absorbtion_factor,
				len)*dry,len/343.0f,len, band, keyframeID);
		}
		}

		const float gain = currentSound->getGain();
		rec->Multiply(gain*gain);

	}

}
Scene::~Scene() {
	{std::vector<Recorder*>::const_iterator it = listeners.begin();
	for ( ; it != listeners.end(); ++ it ) {
		delete *it;
	}}
	{std::vector<AbstractSoundFile*>::const_iterator it =
		sources.begin();
	for ( ; it != sources.end(); ++ it ) {
		delete *it;
	}}
	{std::vector<Mesh*>::const_iterator it = meshes.begin();
	for ( ; it != meshes.end(); ++ it ) {
		delete *it;
	}}
}