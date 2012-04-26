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
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <gmtl/Intersection.h>

#include "Mesh.h"
#include "HelperFunctions.h"
#include "Triangle.h"
#include "Material.h"

bool Mesh::RayIntersection(gmtl::Rayf* r,gmtl::Point3f* &p, gmtl::Vec3f* &n, Material* &mat) {
	float d = 1000000;
	float u,v,t;
	bool x = false;
	Triangle* tri;
	std::vector<Triangle*>::const_iterator ti;
	for(ti=tris.begin(); ti!=tris.end(); ++ti){
		if ( gmtl::intersectDoubleSided(**ti,*r,u,v,t) && t > 0.001f && t < d ) {
			d = t;
			tri = *ti;
			x = true;
		}
	}
	if ( !x ) return false;
	p = new gmtl::Point3f();
	*p = r->mOrigin + r->mDir * d;
	if ( gmtl::dot(tri->normal,r->mDir) > 0.0 ) {
		n = new gmtl::Vec3f(tri->normal * -1.0f);
	} else {
		n = new gmtl::Vec3f(tri->normal);
	}
	mat = tri->m;
	return x;
}

bool Mesh::LineIntersection(gmtl::LineSegf* l) {
	float u,v,t;
	std::vector<Triangle*>::const_iterator ti;
	for(ti=tris.begin(); ti!=tris.end(); ++ti){
		// intersectDoubleSided is only defined for the gmtl::Ray type, therefore
		// we need to check ourselves if t is within (0,1], which is the parametric
		// range for the gmtl::LineSeg type.
		if ( gmtl::intersectDoubleSided(**ti,*l,u,v,t) && t > 1e-5f && t < 1.0f ) {
			return true;
		}

	}
	return false;
}

Mesh* Mesh::Empty() {
	return new Mesh(false);
}

Mesh::Mesh(bool from_file) {
	total_area = 0;
	total_weighted_area = 0;
	if ( ! from_file ) return;
	Read(false);
	assertid("MESH");		
	std::string m = ReadString();
	material = materials[m];
	const float absorption = 1.0f - material->absorption_coefficient[1];
	while ( Datatype::PeakId() == "tri " ) {
		Triangle* tri = new Triangle();
		tri->m = material;
		tris.push_back(tri);
		total_area += tri->area;
		total_weighted_area += tri->area * absorption;
	}
	has_boundingbox = false;
	BoundingBox();
	std::stringstream ss;
	ss << std::setprecision(std::cout.precision()) << std::fixed;
	std::string indent = std::string(" ",Datatype::prefix.size());
	ss << Datatype::prefix << "Mesh \r\n" << indent << " +- faces: " << tris.size() << std::endl << indent << " +- material: '" << material->name << "'" << std::endl;
	ss << indent << " +- bounds: (" << xmin << ", " << ymin << ", " << zmin << ") - (" << xmax << ", " << ymax << ", " << zmax << ")" << std::endl;
	ss << indent << " +- surface area: " << total_area << std::endl;
	ss << indent << " +- total absorption: " << total_weighted_area << std::endl;
	ss << indent << " +- volume: " << Volume() << std::endl;
	if ( indent.size() ) {
		Datatype::stringblock = ss.str();
	} else {
		std::cout << ss.str();
	}
}

Mesh::~Mesh() {
	for ( std::vector<Triangle*>::const_iterator it = tris.begin(); it != tris.end(); ++ it ) {
		delete *it;
	}	
}

void Mesh::Combine(Mesh* m) {
	std::vector<Triangle*>::const_iterator ti;
	for( ti = m->tris.begin(); ti != m->tris.end(); ++ ti ) {
		tris.push_back(*ti);
	}
	total_area += m->total_area;
	BoundingBox();
}

void Mesh::BoundingBox() {
	xmin = ymin = zmin = 1e9;
	xmax = ymax = zmax = -1e9;
	std::vector<Triangle*>::const_iterator it;
	for( it = tris.begin(); it != tris.end(); it ++ ) {
		Triangle* t = *it;
		for ( int i = 0; i < 3; i ++ ) {
			if ( t->mVerts[i][0] < xmin ) xmin = t->mVerts[i][0];
			if ( t->mVerts[i][1] < ymin ) ymin = t->mVerts[i][1];
			if ( t->mVerts[i][2] < zmin ) zmin = t->mVerts[i][2];
			if ( t->mVerts[i][0] > xmax ) xmax = t->mVerts[i][0];
			if ( t->mVerts[i][1] > ymax ) ymax = t->mVerts[i][1];
			if ( t->mVerts[i][2] > zmax ) zmax = t->mVerts[i][2];
		}
	}
	has_boundingbox = true;
}

void Mesh::SamplePoint(gmtl::Point3f& p, gmtl::Vec3f& n) {
	float x = gmtl::Math::rangeRandom(0,total_area);
	std::vector<Triangle*>::const_iterator it;
	for( it = tris.begin(); it != tris.end(); it ++ ) {
		x -= (*it)->area;
		if ( x < 0 ) {
			(*it)->SamplePoint(p);
			n = (*it)->normal;
			break;
		}
	}
}

float Mesh::Area() const {
	return total_area;
}

float Mesh::TotalAbsorption() const {
	return total_weighted_area;
}

float Mesh::Volume() const {
	// http://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
	float volume = 0.0f;
	std::vector<Triangle*>::const_iterator it;
	for( it = tris.begin(); it != tris.end(); it ++ ) {
		volume += (*it)->SignedVolume();
	}
	return volume;
}

float Mesh::AverageAbsorption() const {
	return total_weighted_area / total_area;
}

std::map<std::string,Material*> Mesh::materials;
