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

#include <iostream>
#include <string>
#include <stdexcept>

#include "Material.h"

Material::Material() {
	Read(false);
	assertid("MAT ");
	name = ReadString();
	std::cout << "Material '" << name << "'" << std::endl;
	std::cout << " +- refl:   [";
	for( int i = 0; i < 3; i ++ )
		absorption_coefficient[i] = 1.0f;
	for( int i = 0; i < 3; i ++ ) {
		const float f = ReadFloat();
		reflection_coefficient[i] = f;
		std::cout << f;
		if ( i < 2 ) std::cout << ", ";
		absorption_coefficient[i] -= f - 1e-9f;
	}
	std::cout << "]" << std::endl;
	if ( Datatype::PeakId() == "flt4" ) {
		std::cout << " +- trans:  [";
		for( int i = 0; i < 3; i ++ ) {
			const float f = ReadFloat();
			refraction_coefficient[i] = f;
			std::cout << f;
			if ( i < 2 ) std::cout << ", ";
			absorption_coefficient[i] -= f - 1e-9f;
		}
		std::cout << "]" << std::endl;
	}
	std::cout << " +- absorp: [";
	for( int i = 0; i < 3; i ++ ) {
		const float f = absorption_coefficient[i];
		if ( f < 0.0f ) throw DatatypeException("Invalid material settings");
		std::cout << f;
		if ( i < 2 ) std::cout << ", ";
		absorption_coefficient[i] = 1.0f-f;
	}
	std::cout << "]" << std::endl;
	if ( Datatype::PeakId() == "flt4" ) {
		std::cout << " +- spec:   [";
		for( int i = 0; i < 3; i ++ ) {
			const float f = ReadFloat();
			specularity_coefficient[i] = f;
			std::cout << f;
			if ( i < 2 ) std::cout << ", ";
		}
		std::cout << "]" << std::endl;
	}
}
bool Material::isTransparent() {
	return refraction_coefficient.size() == 3;
}
BounceType Material::Bounce(int band) {
	const float fl = reflection_coefficient[band];
	const float fr = refraction_coefficient[band];
	if ( fl < 0.0001 && fr < 0.0001 ) return REFLECT;
	const float ab = 1.0f - fl - fr;
	const float fl2 = fl / (fl+fr);
	return ( gmtl::Math::unitRandom() <= fl2 ) ? REFLECT : REFRACT;
}