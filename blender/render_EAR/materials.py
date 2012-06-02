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
# Sample material definitions, inspired by the Odeon Material.Li8 file #
#                                                                      #
########################################################################

mats = """
100% Transparent
 0.00000 0.00000 0.00000
 1.00000 1.00000 1.00000
 1.00000 1.00000 1.00000

100% Absorbent
 0.00000 0.00000 0.00000
 0.00000 0.00000 0.00000
 0.50000 0.50000 0.50000

100% Reflecting
 1.00000 1.00000 1.00000
 0.00000 0.00000 0.00000
 0.20000 0.40000 0.80000

Rough concrete
 0.98000 0.96000 0.93000
 0.00000 0.00000 0.00000
 0.10000 0.20000 0.40000

Smooth concrete
 0.99000 0.98000 0.95000
 0.00000 0.00000 0.00000
 0.30000 0.60000 0.90000

Wooden floor
 0.80000 0.90000 0.95000
 0.15000 0.08000 0.04000
 0.30000 0.40000 0.50000

Glazing
 0.60000 0.70000 0.80000
 0.39000 0.24000 0.17000
 0.60000 0.70000 0.90000
"""

definitions = []

def materials():
    global mats, definitions
    names = ["-- Custom --"]
    definitions.append(None)
    data = []
    for l in [x for x in mats.split('\n') if x != '' and x[0] != '#']:
        if len(data) == 3:
            definitions.append(data)
            data = []
            names.append(l)
        elif len(names)>len(definitions):
            data.append([float(x) for x in l.split()])            
        else:
            names.append(l)
    definitions.append(data)
    return list(zip([str(x) for x in range(len(names))],names,names))

def lookup(str):
    global definitions
    return definitions[int(str)]