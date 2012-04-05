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

#include "Settings.h"
#include "Datatype.h"

void Settings::init(Datatype* d) {
	Datatype::scanning = true;
	char* old_input = Datatype::input;
	int old_input_length = Datatype::input_length;
	Datatype::input = d->data;
	std::cout << "Settings" << std::endl;
	while( Datatype::PeakId() == "str " ) {
		std::string str = Datatype::ReadString();
		std::cout << " +- " << str << ": ";
		Datatype* D = Datatype::Read();
		settings[str] = D;
		if ( D->isFloat() )	std::cout << *((float*)D->data);
		else if ( D->isInt() ) std::cout << *((int*)D->data);
		else if ( D->isVec() ) {
			float* f = (float*)D->data;
			std::cout << "[" << *(f+1) << ", " << *(f+3) << ", " << *(f+5) << "]";
		} else if ( D->isString() ) {
			int j;
			for( int i = 0;; i ++ ) {
				if ( D->data[i] == 0 ) { j = i; break; }
			}
			std::string s = std::string(D->data,j);
			std::cout << s;
		}
		std::cout << std::endl;
	}
	Datatype::input = old_input;
	Datatype::input_length = old_input_length;
	Datatype::scanning = false;
}


Datatype* Settings::getsetting(const std::string& s, int warn) {
	boost::mutex::scoped_lock lock(m_mutex);
	std::map<std::string,Datatype*>::const_iterator it=settings.find(s);
	if ( it == settings.end() ) {
		if ( warn != SETTING_NOTFOUND_IGNORE ) {
			std::stringstream ss;
			ss << "Setting '" << s << "' not found";
			if ( warn == SETTING_NOTFOUND_WARN && std::find ( errors.begin(), errors.end(), s ) == errors.end() ) {
				std::cout << ss.str() << std::endl;
				errors.push_back(s);
			} else if ( warn == SETTING_NOTFOUND_THROW ) {
				throw DatatypeException(ss.str());
			}
		}
		return 0;
	}
	return it->second;		
}

int Settings::GetInt(const std::string& s) {
	Datatype* d = getsetting(s, SETTING_NOTFOUND_THROW);
	d->assertid("int4");
	return *((int*) d->data);
}
bool Settings::GetBool(const std::string& s) {
	return GetInt(s) > 0;
}
bool Settings::IsSet(const std::string& s) {
	return !!getsetting(s, SETTING_NOTFOUND_IGNORE);
}
float Settings::GetFloat(const std::string& s) {
	Datatype* d = getsetting(s);
	d->assertid("flt4");
	return * ((float*) d->data);
}
gmtl::Vec3f Settings::GetVec(const std::string& s) {
	Datatype* d = getsetting(s, SETTING_NOTFOUND_THROW);
	Datatype::push(d);
	gmtl::Vec3f v = Datatype::ReadVec();
	Datatype::pop();
	return v;
}
std::string Settings::GetString(const std::string& s) {
	Datatype* d = getsetting(s, SETTING_NOTFOUND_THROW);
	Datatype::push(d);
	std::string v = Datatype::ReadString();
	Datatype::pop();
	return v;
}

std::map<std::string,Datatype*> Settings::settings;
std::vector<std::string> Settings::errors;
boost::mutex Settings::m_mutex;
