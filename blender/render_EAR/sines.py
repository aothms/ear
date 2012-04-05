########################################################################
#                                                                      #
# This file is part of EAR: Evaluation of Acoustics using Ray-tracing. #
#                                                                      #
# EAR is free software: you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by #
# the Free Software Foundation, either version 3 of the License, or    #
# (at your option) any later version.                                  #
#                                                                      #
# EAR is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of       #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        #
# GNU General Public License for more details.                         #
#                                                                      #
# You should have received a copy of the GNU General Public License    #
# along with EAR.  If not, see <http://www.gnu.org/licenses/>.         #
#                                                                      #
########################################################################

########################################################################
#                                                                      #
# A module to create output a sine complex based on either a base      #
# frequency and several overtones or based on a list of absolute       #
# frequencies.                                                         #
#                                                                      #
########################################################################

import wave
import array
from math import sin,pi
try: from functools import reduce
except: pass

# A function in the form of f(x) = a*sin(2pi*f)
class Sine:
	rate = 22050.0 / pi
	def __init__(self,p,a):
		self.p = p
		self.a = a
		self.l = [self]
	def __neg__(self):
		return Sine(self.p,-self.a)
	def __add__(self,b):
		a = Sine(self.p,self.a)
		a.l = self.l[:]
		a.l.append(b)
		return a
	def __str__(self):
		return "+".join("%.2f*sin(%d*2pi)"%(x.a,x.p) for x in self.l)
	def __sub__(self,b):
		return self + (-b)
	def __getitem__(self,k):
		if type(k) == int:
			return sum([sin(float(s.p*k)/self.rate)*self.a for s in self.l])
		elif type(k) == slice:
			return [self[i] for i in range(*k.indices(k.stop))]

# A function in the form of:
#        |     (x/a)^ae for x <= a
# f(x) = | ((x-a)/d)^de for x > a
# To specify the envelope, attack and decay of the sine complex
class Envelope:
	def __init__(self,a,ae,d,de):
		self.a = a * 44100.0
		self.d = d * 44100.0
		self.ae, self.de = ae,de
	def __iter__(self):
		return iter([(x/self.a)**self.ae for x in range(int(self.a))]+\
		[(x/self.d)**self.de for x in range(int(self.d)-1,0,-1)])

# A class to write the samples to file
class Wave:	
	def __init__(self,fn,s):
		maxh = 2**15-1
		d = array.array('h',[maxh if r >= 1.0 else -maxh if r <-1.0 else int(r*maxh) for r in s])
		w = wave.open(fn,'wb')
		w.setparams((1,2,44100,0,'NONE','not compressed'))
		w.writeframes(d.tostring())

# Outputs a sine complex based on a list of absolute frequencies
def write_compound(filename,attack,attack_exp,decay,decay_exp,frequencies):
	envelope = list(Envelope(attack,attack_exp,decay,decay_exp))
	sines = reduce(Sine.__add__,[Sine(a,b) for a,b in frequencies])
	sine_data = sines[0:len(envelope)]
	wave_data = [a*b for a,b in zip(envelope,sine_data)]
	Wave(filename,wave_data)	
	
# Outputs a sine complex based on a base frequency and several overtones
def write_compound_overtones(filename,attack,attack_exp,decay,decay_exp,base,overtones):
	write_compound(filename,attack,attack_exp,decay,decay_exp,[(base*2.0**a,b) for a,b in overtones])
    