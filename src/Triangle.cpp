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

#include <gmtl/Tri.h>

#include "Datatype.h"
#include "Material.h"
#include "Triangle.h"

void Triangle::calcArea() {
	gmtl::Vec3f result;
	gmtl::cross(result,edge(0),edge(1));
	area = gmtl::length(result) / 2.0f;
}
Triangle::Triangle(const gmtl::Point3f& a,const gmtl::Point3f& b,const gmtl::Point3f& c) : gmtl::Trif(a,b,c) {
	normal = gmtl::normal(*this);
	calcArea();
}
Triangle::Triangle() : gmtl::Trif() {
	Read(false);
	assertid("tri ");
	mVerts[0] = ReadVec();
	mVerts[1] = ReadVec();
	mVerts[2] = ReadVec();
	normal = gmtl::normal(*this);
	calcArea();
}
void Triangle::SamplePoint(gmtl::Point3f& P) {
	// http://math.stackexchange.com/questions/18686/uniform-random-point-in-triangle
	const float r1 = gmtl::Math::unitRandom();
	const float r2 = gmtl::Math::unitRandom();
	const float sr1 = gmtl::Math::sqrt(r1);
	const gmtl::Point3f& A = (*this)[0];
	const gmtl::Point3f& B = (*this)[1];
	const gmtl::Point3f& C = (*this)[2];
	P = (1.0f - sr1) * A + (sr1 * (1.0f - r2)) * B + (sr1 * r2) * C;
}

float Triangle::SignedVolume() const {
	// http://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
	const gmtl::Point3f& p1 = mVerts[0];
	const gmtl::Point3f& p2 = mVerts[1];
	const gmtl::Point3f& p3 = mVerts[2];
	const float v321 = p3[0]*p2[1]*p1[2];
    const float v231 = p2[0]*p3[1]*p1[2];
    const float v312 = p3[0]*p1[1]*p2[2];
    const float v132 = p1[0]*p3[1]*p2[2];
    const float v213 = p2[0]*p1[1]*p3[2];
    const float v123 = p1[0]*p2[1]*p3[2];
    return (1.0f/6.0f)*(-v321 + v231 + v312 - v132 - v213 + v123);
}