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
# Maps the animated location of a blender object to a soundtrack       #
#                                                                      #
########################################################################

import bpy
import wave
import mathutils
import glob
import random
import struct
import math

# Note: ideally these classes should be updated to use the Python array type

# A class to read the samples from a .wav file
class SoundFile():
    def __init__(self,filename='in.wav'):
        self.filename = bpy.path.abspath(filename)
        self.wavefile = wave.open(self.filename,'rb')
        self.nchannels, self.sampwidth,\
        self.framerate, self.nframes,\
        self.comptype, self.compname = self.wavefile.getparams()
        self.samples = [stof(i) for i in struct.unpack("%dh"%self.nframes, self.wavefile.readframes(self.nframes))]
        self.wavefile.close()
        
# A class that allocated a list of floats to store a sequence of sonic events
class Recorder():
    def __init__(self):
        self.samples = []
        self.framerate = 44100
    def stretch(self,l):
        if l > len(self.samples):
            self.samples += [0.0] * (l-len(self.samples)+1)
    def add(self,sf,offset=0.0):
        t1 = int(offset*self.framerate)
        t2 = t1 + len(sf.samples)
        self.stretch(t2)
        for i in range(len(sf.samples)):
            self.samples[i+t1] += sf.samples[i]
    def write(self,fn):
        wavefile = wave.open(fn,'wb')
        wavefile.setparams((1,2,self.framerate,0,'NONE','not compressed'))
        data = [iclamp(stoi(f)) for f in self.samples]
        _data = b''.join((struct.pack('h',d) for d in data))
        wavefile.writeframes(_data)
        wavefile.close()
    def __len__(self):
        return len(self.samples)

# Clamps an integer between a min and a max value
def iclamp(i,min=-32768,max=32767):
    return min if i < min else i if i < max else max

# Maps a floating point value to an unsigned short integer
def stoi(f):
    return int((f)*2.0**15.0)

# Maps an unsigned short integer to a floating point value
def stof(i):
    return float(i)/2.0**15.0

# Sees if the object passes through a portal mesh by
# seeing if the sign of the dot project with a movement
# vector and a face normal flips. And whether a line segment
# from previous location to location intersects with the mesh.
def changed_orientation(loc,object,face,previous,previous_loc):
    location = object.matrix_world.inverted() * loc
    difference = (location-face.center).normalized()
    dot = difference.dot(face.normal)
    facing = dot < 0.0
    if previous is None:
        return False,facing,-1
    if facing != previous:
        previous_location = object.matrix_world.inverted() * previous_loc
        hit_location, normal, face_index = object.ray_cast(previous_location,location)
        if face_index != -1:
            return True,facing,face_index
    return False,facing,-1

    
# Generates a .wav file that contains the sonic events that
# an object encounters along its animated path. When distance
# of step_length is traversed a footstep event is added to,
# when a portal object is passes its corresponding sound is
# added.
def generate(biped, step_length=0.65):
    scn = bpy.context.scene
    
    major,minor = bpy.app.version[0:2]
    transpose_matrices = minor >= 62
    if transpose_matrices:
        location = mathutils.Vector([r[3] for r in biped.matrix_world[0:3]])
    else:
        location = mathutils.Vector(biped.matrix_world[3][0:3])
        
    old_location = None
    
    rec = Recorder()
    
    fcurves = biped.animation_data.action.fcurves
    
    frame_resolution = 10.0
    
    steps = []
    
    portals = []
    for mesh in [m for m in bpy.data.objects if m.is_portal]:
        portals.extend(zip([mesh]*len(mesh.data.faces),list(mesh.data.faces)))
    portal_orientations = [None]*len(portals)
    
    for fr in range(scn.frame_start*int(frame_resolution),(scn.frame_end+1)*int(frame_resolution)):
        frame = fr / frame_resolution
        for crv in [c for c in fcurves if c.data_path == 'location']:
            location[crv.array_index] = crv.evaluate(frame)
        if not old_location:
            old_location = location.copy()
            continue
        t = frame/float(scn.render.fps)
        portal_index = 0
        for portal_object,portal_face in portals:
            changed,facing,face_index = changed_orientation(location,portal_object,portal_face,portal_orientations[portal_index],old_location)
            portal_orientations[portal_index] = facing
            portal_index += 1
            if changed:
                sound_file = SoundFile(portal_object.filename)
                rec.add(sound_file,t)                
        distance = (location-old_location).length
        if distance > step_length:
            old_location = location.copy()
            steps.append((t,location.copy()))
            
    meshes = [m for m in scn.objects if m.type == 'MESH']
    
    step_locations = []
    
    for t,ear_location in steps:
        closest_distance = float('+inf')
        closest_object = None
        closest_location = None
        face_index = -1
        for m in meshes:
            ray_start = ear_location * m.matrix_world.inverted()
            ray_end = ray_start + mathutils.Vector([0,0,-2])
            hit_location, normal, mi = m.ray_cast(ray_start,ray_end)
            if mi != -1:
                face_index = mi
                hit_location_global = hit_location * m.matrix_world
                distance = (ear_location-hit_location_global).length
                if distance < closest_distance:
                    closest_distance = distance
                    closest_object = m
                    closest_location = hit_location_global + mathutils.Vector([0,0,0.01])
        if closest_object:
            step_locations.append(closest_location)
            material_index = closest_object.data.faces[face_index].material_index
            material = closest_object.material_slots[material_index].material
            steps_dir = material.stepdir
            step_files = glob.glob("%s/*.wav"%bpy.path.abspath(steps_dir))
            if not len(step_files): continue
            sound = step_files[random.randrange(0,len(step_files))]
            sound_file = SoundFile(sound)
            rec.add(sound_file,t)
            
    if len(rec):
        num_keyframes = int(math.ceil(scn.frame_end-scn.frame_start)/scn.frame_step)
        key_spacing = len(steps) // (num_keyframes)
        locs = []
        for i in range(0,num_keyframes):
            locs.append(step_locations[i*key_spacing])
        locs.append(step_locations[-1])
        out_file = bpy.path.abspath('//%s.%d-%d.wav'%(biped.name,scn.frame_start,scn.frame_end))
        rec.write(out_file)
        return out_file,locs
    else: return None, None