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

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include <gmtl/Intersection.h>

#include "HelperFunctions.h"
#include "Triangle.h"
#include "Material.h"

/// This class defines a set of triangles that together make an object
/// that reflects sound rays. The volume does not need to be closed and
/// triangles are defined two-sides, which means that they reflect sound
/// regardless of whether the dot product of the ray direction and the
/// triangle normal is greater or larger than zero. Only a single material
/// can be assigned to a mesh. A mesh can also be used as an emitting
/// volume for area sound sources.
class Mesh : public Datatype {
private:
	bool has_boundingbox;
	float total_area;
	float total_weighted_area;
public:
	std::vector<Triangle*> tris;
	Material* material;

	static std::map<std::string,Material*> materials;
	
	float xmin,ymin,zmin,xmax,ymax,zmax;

	bool RayIntersection(gmtl::Rayf* r,gmtl::Point3f* &p, gmtl::Vec3f* &n, Material* &mat) ;
	bool LineIntersection(gmtl::LineSegf* l);
	void SamplePoint(gmtl::Point3f& p, gmtl::Vec3f& n);
	Mesh(bool from_file = true);
	~Mesh();
	static Mesh* Empty();
	void Combine(Mesh* m);
	void BoundingBox();
	/// Returns the surface area of the mesh, useful for example to determine the T60
	/// reverberation time using Sabine, Eyring or Millington-Sette.
	float Area() const;
	/// Returns the surface area of the mesh times the average absorption of the
	/// surfaces, commonly called the Total Absorption measured in Sabins. Useful
	/// for example to determine the T60 reverberation time using Sabine, Eyring
	/// or Millington-Sette.
	float TotalAbsorption() const;
	/// Calculates the internal volume of the mesh. In case of a non-manifold or open
	/// mesh, this function returns wrong results. For example to determine the T60
	/// reverberation time using Sabine, Eyring or Millington-Sette.
	float Volume() const;
	/// Calculates the internal volume of the mesh. In case of a non-manifold or open
	/// mesh, this function returns wrong results. For example to determine the T60
	/// reverberation time using Sabine, Eyring or Millington-Sette.
	float AverageAbsorption() const;
};

#endif