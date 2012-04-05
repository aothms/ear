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

#ifndef MATERIAL_H
#define MATERIAL_H

#include <map>

#include "Datatype.h"

enum BounceType { REFLECT, REFRACT, ABSORB };

/// This class represents the material of mesh surfaces and defines how
/// sound rays bounce off of the material.
class Material : public Datatype {
public:
	std::string name;
	std::map<int,float> reflection_coefficient;
	std::map<int,float> refraction_coefficient;
	std::map<int,float> absorption_coefficient;
	std::map<int,float> specularity_coefficient;
	std::map<int,float>::iterator it;

	Material();
	bool isTransparent();
	BounceType Bounce(int band);
};

#endif