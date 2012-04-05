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

#include <sstream>

#include "Datatype.h"
#include "Settings.h"

Datatype::Datatype() {
	id = (int*)input;
	data = input + 4;
	if ( isInt() || isFloat() || isVec() || isString() || isTri() ) {
		length = 0;
	} else {
		length = (int*)(input+4);
		data += 4;
	}	
	char* aid = (char*)id;
	if ( !scanning && Settings::GetBool("debug") ) 
		std::cout << "Reading '" << aid[0] << aid[1] << aid[2] << aid[3] << "' block of " << *length << " bytes" << std::endl;
}
void Datatype::assertid(const char* c) {
	if ( *((int*)c) != *id ) {
		char* aid = (char*)id;
		std::stringstream ss;
		ss << "Found '";
		ss.write(aid,4);
		ss << "' while expecting '" << c << "'";
		DatatypeException(ss.str());
	}
}
bool Datatype::isInt() {
	return *id == *((int*)"int4");
}
bool Datatype::isFloat() {
	return *id == *((int*)"flt4");
}
bool Datatype::isString() {
	return *id == *((int*)"str ");
}
bool Datatype::isVec() {
	return *id == *((int*)"vec3");
}
bool Datatype::isTri() {
	return *id == *((int*)"tri ");
}
void Datatype::push(Datatype* d) {
	readpos r;
	r.input = input;
	r.input_length = input_length;
	input = d->data - (d->length ? 8 : 4);
	input_length = d->length ? (*d->length + 8) : 0;
	readstack.push_back(r);
}
void Datatype::pop() {
	readpos r = readstack.back();
	readstack.pop_back();
	input = r.input;
	input_length = r.input_length;
}
std::string Datatype::PeakId() {
	return std::string (input,4);
}
bool Datatype::SetInput(std::string filename) {
	std::ifstream f (filename.c_str(), std::ios_base::binary);
	if ( ! f.good() ) return false;
	f.seekg(0,std::ios_base::end);
	input_length = f.tellg();
	f.seekg(0,std::ios_base::beg);
	buffer = input = new char[input_length];
	f.read(input,input_length);
	if ( Datatype::PeakId() != ".EAR" ) return false;
	input += 4;input_length -= 4;
	return true;
}
void Datatype::Dispose() {
	delete[] buffer;
}
Datatype* Datatype::Read(bool r) {
	Datatype* d = new Datatype();
	const int header_size = d->length ? 8 : 4;
	int length = 0;
	if ( d->length ) length = *d->length;
	else if ( d->isFloat() || d->isInt() ) length = 4;
	else if ( d->isVec() ) length = 24;
	else if ( d->isTri() ) length = 24*3;
	else if ( d->isString() ) {
		char* temp = d->data;
		int max_length = input_length - header_size + 4;
		while ( *(temp++) && (max_length--) ) length ++;
		length += 4 - (length%4);
	}
	input += header_size;input_length -= header_size;
	if ( r ) { input += length; input_length -= length; }
	return d;
}
float Datatype::ReadFloat() {
	Datatype* d = Read();
	d->assertid("flt4");
	float f = *((float*)d->data);
	delete d;
	return f;
}
gmtl::Vec3f Datatype::ReadVec() {
	Datatype* d = Read(false);
	d->assertid("vec3");
	float x = ReadFloat();
	float y = ReadFloat();
	float z = ReadFloat();
	return gmtl::Vec3f(x,y,z);
}
gmtl::Point3f Datatype::ReadPoint() {
	Datatype* d = Read(false);
	d->assertid("vec3");
	float x = ReadFloat();
	float y = ReadFloat();
	float z = ReadFloat();
	return gmtl::Point3f(x,y,z);
}
std::string Datatype::ReadString() {
	Datatype* d = Read();
	d->assertid("str ");
	int length = 0;
	char* temp = d->data;
	int max_length = input_length + 4;
	while ( *(temp++) && (max_length--) ) length ++;
	std::string s = std::string(d->data,length);
	delete d;
	return s;
}
Datatype* Datatype::Scan(std::string a) {
	scanning = true;
	char* old_input = input;
	int old_input_length = input_length;
	Datatype* ret = 0;
	while( Datatype::input_length ) {
		const bool b = Datatype::PeakId() == a;
		Datatype* d = Datatype::Read();
		if ( b ) { ret = d; break; }
		delete d;
	}
	input = old_input;
	input_length = old_input_length;
	scanning = false;
	return ret;		
}
char* Datatype::input = 0;
char* Datatype::buffer = 0;
std::string Datatype::prefix = "";
std::string Datatype::stringblock = "";
int Datatype::input_length = 0;
bool Datatype::scanning = false;
std::vector<readpos> Datatype::readstack;
