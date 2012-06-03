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

#ifndef TRIANGLE_H_
#define TRIANGLE_H_

#include <gmtl/Tri.h>

#include "Datatype.h"
#include "Material.h"

/// A simple extention to the gmtl Trif class to also store the triangle
/// area and normal and add a function to sample a point on the triangle
/// using a uniform distribution, the latter is used when a mesh is used
/// as an emitting surface for a sound source.
class Triangle : public gmtl::Trif, public Datatype {
private:
	void calcArea();
public:
	gmtl::Vec3f normal;
	float area;
	Material* m;
	Triangle(const gmtl::Point3f& a,const gmtl::Point3f& b,const gmtl::Point3f& c);
	Triangle();
	void SamplePoint(gmtl::Point3f& p);
	float SignedVolume() const;
};

#endif