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

#include <math.h>

#include "Equalizer.h"

void Equalizer::Pass::Process(const float in, float &out) {
	out = a0 * in + a1 * xm1 + a2 * xm2 + a3 * xm3 + a4 * xm4 - b1 * ym1 - b2 * ym2 - b3 * ym3 - b4 * ym4;
	xm4 = xm3; xm3 = xm2; xm2 = xm1; xm1 = in;
	ym4 = ym3; ym3 = ym2; ym2 = ym1; ym1 = out;
}
Equalizer::Pass::Pass(float fc) {
	//fc -> cutoff frequency
	//srate -> sample rate

	const float srate = 44100.0f;
	const float pi = 3.14285714285714f;

	// shared for both lp, hp; optimizations here
	float wc  = 2.0f * pi * srate;
	float wc2 = wc * wc;
	float wc3 = wc2 * wc;
	      wc4 = wc2 * wc2;
	float k  = wc / tan(pi * fc / srate);
	float k2 = k * k;
	float k3 = k2 * k;
	      k4 = k2 * k2;
	float sqrt2 = sqrtf(2.0f);
	float sq_tmp1 = sqrt2 * wc3 * k;
	float sq_tmp2 = sqrt2 * wc * k3;
	      a_tmp = 4.0f * wc2 * k2 + 2.0f * sq_tmp1 + k4 + 2.0f * sq_tmp2 + wc4;

	b1 = (4.0f * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
	b2 = (6.0f * wc4 - 8.0f * wc2 * k2 + 6.0f * k4) / a_tmp;
	b3 = (4.0f * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
	b4 = (k4 - 2.0f * sq_tmp1 + wc4 - 2.0f * sq_tmp2 + 4.0f * wc2 * k2) / a_tmp;

	xm1 = xm2 = xm3 = xm4 = 0.0f;
	ym1 = ym2 = ym3 = ym4 = 0.0f;
}

Equalizer::LowPass::LowPass(float fc) : Equalizer::Pass(fc) {
	a0 = wc4 / a_tmp;
	a1 = 4.0f * wc4 / a_tmp;
	a2 = 6.0f * wc4 / a_tmp;
	a3 = a1;
	a4 = a0;
}

Equalizer::HighPass::HighPass(float fc) : Equalizer::Pass(fc) {
	a0 = k4 / a_tmp;
	a1 = -4.0f * k4 / a_tmp;
	a2 = 6.0f * k4 / a_tmp;
	a3 = a1;
	a4 = a0;
}

void Equalizer::Split(float* data, float* low, float* mid, float* high, unsigned int length, float f1, float f2, float f3) {
	const float fc1 = (f1+f2)/2.0f;
	const float fc2 = (f2+f3)/2.0f;
	HighPass hi_pass1 = fc1;
	HighPass hi_pass2 = fc2;
	LowPass lo_pass1 = fc1;
	LowPass lo_pass2 = fc2;
	for ( unsigned int i = 0; i < length; ++ i ) {
		hi_pass2.Process(data[i], high[i]);
		lo_pass1.Process(data[i], low[i]);
		lo_pass2.Process(data[i], mid[i]);
		hi_pass1.Process(mid[i], mid[i]);
	}
}