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
#include <string>
#include <stdexcept>

#include <boost/thread/mutex.hpp>

#include <gmtl/gmtl.h>

#include "HelperFunctions.h"

bool Split(const std::string& str, std::string& a, std::string& b) {
	const std::string& delimiters = " ";
	std::string::size_type pos = str.find_first_of(delimiters);
	if ( pos == std::string::npos ) return false;
	a = str.substr(0,pos);
	b = str.substr(pos+1);
	return true;
}

std::string DirName(const std::string& str) {
	const std::string& delimiters = "\\/";
	std::string::size_type pos = str.find_last_of(delimiters);
	if ( pos == std::string::npos ) throw std::runtime_error("Failed to interpret file path");
	return str.substr(0,pos+1);
}

std::string FileName(const std::string& str) {
	const std::string& delimiters = "\\/";
	std::string::size_type pos = str.find_last_of(delimiters);
	if ( pos == std::string::npos ) throw std::runtime_error("Failed to interpret file path");
	return str.substr(pos+1);
}

boost::mutex cout_mutex;
static int nsegments = 1;
static int segment = 0;
void SetProgressBarSegments(int n) { nsegments = n; segment = 0; }
void NextProgressBarSegment() { ++ segment; }
void DrawProgressBar(const int& done, const int& total) {
	const int progress_update_interval = total / 50;
	if ( ! (done % progress_update_interval) ) {
		boost::mutex::scoped_lock lock(cout_mutex);
		std::cout << "\r[";
		for ( int _x = 0; _x < total - progress_update_interval; _x += progress_update_interval )
			std::cout << ((nsegments*_x<done+segment*total) ? "=" : " ");
		std::cout << "]" << std::flush;
	}
}

void DrawString(const std::string & s) {
	boost::mutex::scoped_lock lock(cout_mutex);
	std::cout << s;
}