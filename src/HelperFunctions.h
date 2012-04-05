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

#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

/// Splits a string into a part before and after the first space encountered
bool Split(const std::string& str, std::string& a, std::string& b);

/// Returns the directory name of a path
std::string DirName(const std::string& str);

/// Returns the filename of a path
std::string FileName(const std::string& str);

/// Sets the number of segments for the progress bar in case the maximum 
/// number of threads is larger than the pool of tasks
void SetProgressBarSegments(int n);
/// Signals work on the first batch of tasks is done and moves to the next
void NextProgressBarSegment();
/// Draw a progress bar to stdout using appropriate locking for threads
void DrawProgressBar(const int& done, const int& total);

/// Draw a string to stdout using appropriate locking for threads
void DrawString(const std::string & s);

#ifdef _MSC_VER
#define DIR_SEPERATOR "\\"
#else
#define DIR_SEPERATOR "/"
#endif

#endif