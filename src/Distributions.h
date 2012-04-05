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

/************************************************************************
 *                                                                      *
 * This file contains some formulas and functions that are related to   *
 * statistical and geometrical distrubutions. Implementations are sub-  *
 * optimal in terms of performance and efficiency.                      *
 *                                                                      *
 ************************************************************************/

#ifndef DISTRIBUTIONS_H
#define DISTRIBUTIONS_H

#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>

#ifdef PI
#undef PI
#endif
#define PI 3.14159265f
#define SHERE_SURFACE(R) (4.0f*PI*(R)*(R))
#define SPHERE_VOLUME(R) (4.0f/3.0f*PI*(R)*(R)*(R))
#define INV_SPHERE (1.0f / SHERE_SURFACE(1.0f))
#define INV_HEMI (2.0f / SHERE_SURFACE(1.0f))
#define INV_SPHERE_2(R) (1.0f / SHERE_SURFACE(R))
#define INV_HEMI_2(R) (2.0f / SHERE_SURFACE(R))

/// Samples a vector on a sphere. The implementation used here is very
/// inefficient. A point in a unit cube is sampled and discarded if
/// it falls outside a sphere with radius one.
inline void Sphere(gmtl::Vec3f& v) {
	const float f1 = gmtl::Math::unitRandom() * 2.0f - 1.0f;
	const float f2 = gmtl::Math::unitRandom() * 2.0f - 1.0f;
	const float f3 = gmtl::Math::unitRandom() * 2.0f - 1.0f;
	v = gmtl::Vec3f(f1,f2,f3);
	const float l = gmtl::lengthSquared(v);
	if ( l < 0.001f || l > 1.0f ) {
		return Sphere(v);
	}
	v /= sqrt(l);
}
/// Samples a vector on a hemisphere aligned by normal vector n, by
/// first sampling a sphere and discarding the sample if the dot
/// product with the normal vector is negative.
inline void Hemi(gmtl::Vec3f& v, const gmtl::Vec3f& n) {
	Sphere(v);
	if ( gmtl::dot(n,v) < 0.0f ) {
		return Hemi(v,n);
	}
}
/// Samples a vector on a hemisphere aligned by normal vector n, but
/// factors in a reflection vector as well, to account for a specular
/// reflection component.
inline void Hemi(gmtl::Vec3f& v, const gmtl::Vec3f& surface_normal,const gmtl::Vec3f& reflection,float factor) {
	Hemi(v,surface_normal);
	v = v * (1.0f - factor) + reflection * factor;
	gmtl::normalize(v);
}
#endif