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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include <vector>
#include <string>

#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"

#include "Datatype.h"

/// The class provides static access to the SET datatype block in the .EAR
/// file.
/// NOTE Accessing data from the settings block might NOT thread safe!
class Settings : public Datatype {
private:
	enum { SETTING_NOTFOUND_IGNORE, SETTING_NOTFOUND_WARN, SETTING_NOTFOUND_THROW };
	static std::map<std::string,Datatype*> settings;
	static std::vector<std::string> errors;
	static boost::mutex m_mutex;
	static Datatype* getsetting(const std::string& s, int warn= SETTING_NOTFOUND_WARN);
public:
	/// Initializes the settings with the block found in the file.
	static void init(Datatype* d);
	/// Gets a settings as an integer.
	static int GetInt(const std::string& s);
	/// Gets a settings as a boolean, which is an integer > 0.
	static bool GetBool(const std::string& s);
	/// Checks if a settings is defined in the file.
	static bool IsSet(const std::string& s);
	/// Gets a settings as a floating point numeral.
	static float GetFloat(const std::string& s);
	/// Gets a settings as a float triplet vector.
	static gmtl::Vec3f GetVec(const std::string& s);
	/// Gets a settings as a float triplet point.
	static std::string GetString(const std::string& s);
};

#endif