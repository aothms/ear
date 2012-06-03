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

// 4th order Linkwitz-Riley filter, adapted from:
// http://www.musicdsp.org/archive.php?classid=3#266

#ifndef EQUALIZER_H
#define EQUALIZER_H

class Equalizer {
private:
	class Pass {
	protected:
		float b1, b2, b3, b4;
		float a0, a1, a2, a3, a4;
		float xm1, xm2, xm3, xm4;
		float ym1, ym2, ym3, ym4;
		float wc4, k4, a_tmp;
		Pass(float fc);
	public:
		void Process(const float in, float &out);
	};
	class LowPass : public Pass {
	public:
		LowPass(float fc);
	};
	class HighPass : public Pass {
	public:
		HighPass(float fc);
	};
public:
	static void Split(float* data, float* low, float* mid, float* high, unsigned int length, float f1, float f2, float f3);
};

#endif