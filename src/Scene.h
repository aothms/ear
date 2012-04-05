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

#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <map>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>
#include <gmtl/LineSeg.h>

#include "Material.h"
#include "Recorder.h"
#include "Distributions.h"

/// This class encapsulates all datatypes in the .EAR file format and provides
/// methods to tracing the rays from the sound sources bouncing off of the 
/// meshes into the recorders. Ideally this class would also create a reference
/// to some sort of grid acceleration structure to speed up the triangle-ray
/// intersection tests, but this is currently not the case.
class Scene {
private:
	/// Intersects the ray in sound_ray with the triangles of the scene's meshes.
	/// In case no hit is found (for example between there is no geometry in that
	/// direction) 0 is returned. The arguments that are passed by reference return
	/// information about the triangle normal on which the ray is reflected, the
	/// path length of the previous origin to the point of intersection, the material
	/// at the hit point and the type of bounce which is to be processed, meaning
	/// whether the ray is reflected or refracted (through a transparent material)
	inline gmtl::Rayf* Bounce(int band, gmtl::Rayf* sound_ray, gmtl::Vec3f*& surface_normal, float& l, Material*& mat, BounceType& bt);
	/// Sees whether there is a free line of sight between the point p and point x.
	/// This is done by testing all triangles in the scene for intersection with the
	/// line segment between p and x until an intersection is found. So this is a
	/// timeconsuming operation that could be sped up by using a grid acceleration 
	/// structure
	inline gmtl::LineSegf* Connect(const gmtl::Point3f* p, const gmtl::Point3f& x);
public:
	std::vector<Recorder*> listeners;
	std::vector<AbstractSoundFile*> sources;
	std::vector<Mesh*> meshes;
	/// Adds a listener to the scene.
	void addListener(Recorder* l);
	/// Adds a sound source to the scene.
	void addSoundSource(AbstractSoundFile* s);
	/// Adds a mesh to the scene. Under the hood all triangles are stored inside a
	/// single mesh object.
	void addMesh(Mesh* m);
	/// Adds a material definition to the scene.
	void addMaterial(Material* m);
	/// Renders an impulse response for the sound file in sound (an index in the
	/// sources vector) for the frequency band specified in band. Multiple recorders
	/// are supported to be rendered simultaneously in which case for every ray-triangle
	/// intersection a connection is sought between the intersection point and
	/// every recorder location. This is more efficient than rendering each recorder
	/// separately, but does come for free either.
	void Render(int band, int sound, float absorbtion_factor, int num_samples, float dry, const std::vector<Recorder*>& rec, int keyframeID = -1);
	~Scene();
};

#endif