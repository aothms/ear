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

#ifndef DATATYPE_H
#define DATATYPE_H

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <exception>
#include <string>

#include <gmtl/gmtl.h>

class DatatypeException : public std::exception {
private:
	std::string error_message;
public:
	DatatypeException(const std::string& msg) {
		error_message = msg;
	}	
	~DatatypeException() throw() {}	
	virtual const char* what() const throw() {
		return error_message.c_str();
	}
};

/// A structure to keep track of the file cursor, which can be pushed on a
/// stack in case nested datatypes are being read in.
struct readpos {
	char* input;
	int input_length;
};

/// The is the global class for all datatypes that can be serialized to the
/// .EAR file format. The class reads all binary data from the file and can
/// then be sequentially queried for the different blocks it contains.
class Datatype {
public:
	int* id;
	char* data;
	int* length;
	static char* buffer;
	static char* input;
	static int input_length;
	static bool scanning;
	static std::vector<readpos> readstack;
	static std::string prefix;
	static std::string stringblock;

	Datatype();
	void assertid(const char* c);
	bool isInt();
	bool isFloat();
	bool isString();
	bool isVec();
	bool isTri();
	static void push(Datatype* d);
	static void pop();
	static std::string PeakId();
	static bool SetInput(std::string filename);
	static void Dispose();
	static Datatype* Read(bool r=true);
	static float ReadFloat();
	static gmtl::Vec3f ReadVec();
	template <typename T>
	static T ReadTriplet() {
		Datatype* d = Read(false);
		d->assertid("vecf");
		float x = ReadFloat();
		float y = ReadFloat();
		float z = ReadFloat();
		return T(x,y,z);
	}
	static gmtl::Point3f ReadPoint();
	static std::string ReadString();
	static Datatype* Scan(std::string a);
};

#endif