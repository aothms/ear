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
# This addon exports a Blender scene to the .EAR file format for an    #
# auditory evaluation of the scene in EAR: Evaluation of Acoustics     #
# using Ray-tracing. A specification of the file format can be found   #
# at http://explauralisation.org/fileformat                            #
#                                                                      #
########################################################################

bl_info = {
    "name": "E.A.R Evaluation of Acoustics using Ray-tracing",
    "description": "Renders the auditory experience of a scene",
    "author": "Thomas Krijnen",
    "blender": (2, 6, 1),
    "api": 42615,
    "location": "Properties > Render",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Render"}
    
import bpy
import bgl
import mathutils
import wave
import struct
import os
import sys
import stat
import inspect
from math import sqrt
from multiprocessing import cpu_count
from collections import Iterable

from bpy.props import *
from bpy.path import abspath
from os.path import normpath

from . import materials
from . import storyboard
from . import sines
from . import glyphs

major,minor = bpy.app.version[0:2]
transpose_matrices = minor >= 62

# A compound property for components of a sine
class EAR_Prop_FrequencyComponent(bpy.types.PropertyGroup):
    frequency = FloatProperty(name="Frequency", default=440.0, min=20.0, max=20000.0)
    harmonic = FloatProperty(name="Harmonic", default=0.0, min=0.0, max=10.0)
    amplitude = FloatProperty(name="Amplitude", default=0.5, min=0.0, max=2.0)
    remove = BoolProperty(name="Remove",default=False)

def register_properties():
    # A whole bunch of properties for the Blender object
    bpy.types.Object.is_emitter = BoolProperty()
    bpy.types.Object.is_tripleband = BoolProperty()
    bpy.types.Object.is_sine = BoolProperty()
    bpy.types.Object.sine_harmonics = BoolProperty()
    bpy.types.Object.sine_base_frequency = FloatProperty(name="Base", default=440.0, min=20.0, max=20000.0)
    bpy.types.Object.sine_frequencies = CollectionProperty(type=EAR_Prop_FrequencyComponent)
    bpy.types.Object.sine_attack = FloatProperty(name="Attack", default=0.1, min=0.0, max=2.0)
    bpy.types.Object.sine_decay = FloatProperty(name="Decay", default=0.1, min=0.0, max=2.0)
    bpy.types.Object.sine_attack_exponent = IntProperty(name="Attack E", default=1, min=0, max=16)
    bpy.types.Object.sine_decay_exponent = IntProperty(name="Decay E", default=1, min=0, max=16)
    bpy.types.Object.is_storyboard = BoolProperty()
    bpy.types.Object.is_stereo = BoolProperty()
    bpy.types.Object.head_size = FloatProperty(min=0.0,max=10.0,default=0.2)
    bpy.types.Object.head_ab_low = FloatProperty(min=0.0,max=1.0,default=0.1)
    bpy.types.Object.head_ab_mid = FloatProperty(min=0.0,max=1.0,default=0.3)
    bpy.types.Object.head_ab_high = FloatProperty(min=0.0,max=1.0,default=0.9)
    bpy.types.Object.is_listener = BoolProperty()
    bpy.types.Object.is_surface = BoolProperty()
    bpy.types.Object.is_portal = BoolProperty()
    bpy.types.Object.gain = FloatProperty(min=0.0,max=10.0,default=1.0)
    bpy.types.Object.step_size = FloatProperty(min=0.1,max=10.0,default=0.65)
    bpy.types.Object.sound_offset = FloatProperty(min=0.0,max=1000.0,default=0.0)
    bpy.types.Object.filename = StringProperty(name="Path", description="Filename for audio file", subtype='FILE_PATH')
    bpy.types.Object.filename2 = StringProperty(name="Path", description="Filename for audio file", subtype='FILE_PATH')
    bpy.types.Object.filename3 = StringProperty(name="Path", description="Filename for audio file", subtype='FILE_PATH')

    # A function to make sure that reflection+refraction of a material stays below 1.0
    def _update_mat(self,context,prop):
        other = {'refl':'refr','refr':'refl'}
        other_prop = '_'.join(other.get(s,s) for s in prop.split('_'))
        this_val = getattr(self,prop)
        other_val = getattr(self,other_prop)
        if this_val + other_val > 1.0:
            setattr(self,other_prop,1.0-val)
    def update_mat(prop): return lambda x,y: _update_mat(x,y,prop)

    # A whole bunch of properties for the Blender material
    bpy.types.Material.aud_preset = EnumProperty(name="Presets",items=materials.materials(),default="0")
    bpy.types.Material.refl_low = FloatProperty(min=0.0,max=1.0,default=0.5,update=update_mat('refl_low'))
    bpy.types.Material.refl_mid = FloatProperty(min=0.0,max=1.0,default=0.5,update=update_mat('refl_mid'))
    bpy.types.Material.refl_high = FloatProperty(min=0.0,max=1.0,default=0.5,update=update_mat('refl_high'))
    bpy.types.Material.refr_low = FloatProperty(min=0.0,max=1.0,default=0.0,update=update_mat('refr_low'))
    bpy.types.Material.refr_mid = FloatProperty(min=0.0,max=1.0,default=0.0,update=update_mat('refr_mid'))
    bpy.types.Material.refr_high = FloatProperty(min=0.0,max=1.0,default=0.0,update=update_mat('refr_high'))
    bpy.types.Material.exp_low = FloatProperty(min=0.0,max=1.0,default=0.0)
    bpy.types.Material.exp_mid = FloatProperty(min=0.0,max=1.0,default=0.0)
    bpy.types.Material.exp_high = FloatProperty(min=0.0,max=1.0,default=0.0)
    bpy.types.Material.stepdir = StringProperty(name="Path", description="Directory with footsteps", maxlen=1024, default="", subtype='DIR_PATH')

    # Returns the number of concurrent threads the system's cpu can work on
    try: num_threads = cpu_count()
    except: num_threads = 1

    # A whole bunch of properties for the Blender scene
    bpy.types.Scene.freq1 = FloatProperty(min=0.01,max=50.0,default=0.3)
    bpy.types.Scene.freq2 = FloatProperty(min=0.01,max=50.0,default=2.0)
    bpy.types.Scene.freq3 = FloatProperty(min=0.01,max=50.0,default=6.0)
    bpy.types.Scene.ab_low = FloatProperty(min=0.0,max=1.0,default=0.01)
    bpy.types.Scene.ab_mid = FloatProperty(min=0.0,max=1.0,default=0.02)
    bpy.types.Scene.ab_high = FloatProperty(min=0.0,max=1.0,default=0.05)
    bpy.types.Scene.drylevel = FloatProperty(min=0.0,max=10.0,default=1.0)
    bpy.types.Scene.num_samples = IntProperty(min=3,max=10,default=5)
    bpy.types.Scene.max_threads = IntProperty(min=0,max=1000,default=num_threads)
    bpy.types.Scene.debug_dir = StringProperty(name="Path", description="Directory for debug output", subtype='DIR_PATH')
    bpy.types.Scene.ear_exec_path = StringProperty(name="Path", description="Path to the EAR executable", subtype='FILE_PATH')
    
    setup_exec_path()

# Writes the scene to filename in the .EAR file format
def export(filename):
    
    scn = bpy.context.scene
    basename = filename.split("\\")[-1].split("/")[-1]
    fs = open(filename,'wb')
    
    def is_iterable(ob):
        return not is_instance(ob,str) and isinstance(ob, Iterable)
    
    # Appends d to s according to the .EAR file format specification    
    def pack(d,s=b''):
        if type(d) == type(0):
            s += b'int4'
            s += struct.pack('i',d)
        elif type(d) == type(0.0):
            s += b'flt4'
            s += struct.pack('f',d)
        elif type(d) == type([]) and len(d) > 1 and len(d) < 10 and type(d[0]) == type(0.0):
            s += ('vec%d'%len(d)).encode('ascii')
            for f in d: s += pack(f)
        elif type(d) == type([]) and type(d[0]) == type([]):
            s += b'tri '
            for f in d: s += pack(f)
        elif type(d) == type(''):
            e = d.encode('utf8')
            a = d + '\x00' * ( 4 - len(e) % 4 )
            s += b'str '
            s += a.encode('utf8')
        elif type(d) == type(b''): return d
        else: raise ValueError(str(type(d)))
        return s
    
    # Writes a variable to the output stream
    def write(d):
        fs.write(pack(d))
    # Serializes the block with header h and arguments p
    def packblock(h,p):
        ret = b''
        if ( type(h)==type(b'') ): ret += h
        else: ret += str(h).encode('ascii')
        b = b''
        for x in p: b += pack(x)
        ret += struct.pack('i',len(b))
        ret += b
        return ret
    # Writes the block with header h and arguments p to the output stream
    def writeblock(h,p):
        if ( type(h)==type(b'') ): write(h)
        else: write(str(h).encode('ascii'))
        b = b''
        for x in p: b += pack(x)
        write(struct.pack('i',len(b)))
        write(b)
    
    # Writes header and version identifier
    write(b'.EAR')
    writeblock('VRSN',[0])
    
    # Writes the materials in the blender file
    for mat in bpy.data.materials:
        # First see if the material is used by any sound reflecting meshes
        if not len([o for o in bpy.context.scene.objects if o.is_surface and mat.name in o.data.materials]):
            continue
        if mat.aud_preset != "0":
            bl = [mat.name]
            bl.extend(y for x in materials.lookup(mat.aud_preset) for y in x)
        else:
            bl = [mat.name,mat.refl_low,mat.refl_mid,mat.refl_high]
            bl.extend((mat.refr_low,mat.refr_mid,mat.refr_high))
            bl.extend((mat.exp_low,mat.exp_mid,mat.exp_high))
        writeblock('MAT ', bl)
        
    # Writes the settings block 
    settings = ['debug',0,'absorption',[scn.ab_low,scn.ab_mid,scn.ab_high],'drylevel',scn.drylevel,'samples',10 ** scn.num_samples,'maxthreads',scn.max_threads]
    debug_dir = normpath(abspath(scn.debug_dir))
    if debug_dir != '.' and os.path.exists(debug_dir): settings.extend(['debugdir',debug_dir])
    writeblock('SET ',settings)
    
    # A function to determines whether ob has suitable fcurves or its parent object
    def is_animated(ob):
        return ob.animation_data and \
            ob.animation_data.action and \
            len(ob.animation_data.action.fcurves) or \
            (ob.parent and is_animated(ob.parent))
    
    # A check whether any object that is being exported is animated
    # If one of them is animated all other objects are threated as
    # animated as well.
    contains_animation = len(
        [o for o in scn.objects if o.type == 'EMPTY' and (
            o.is_storyboard or 
            o.is_listener or 
            o.is_emitter ) and is_animated(o)]) > 0
    
    # Write the global key-frame offsets to the file
    if contains_animation:
        anim_keys = []
        for fr in range(scn.frame_start,scn.frame_end+1,scn.frame_step):
            anim_keys.append((fr-1) / scn.render.fps)
        writeblock('KEYS',anim_keys)
    
    # Write the equalizer frequencies to the file
    writeblock('FREQ',[scn.freq1,scn.freq2,scn.freq3])
    
    # Keep track of the number of sounds and recorders
    num_sounds = 0
    num_recorders = 0
    recorder_names = []
    
    old_frame = scn.frame_current
    
    for ob in scn.objects:
        if ob.type == 'MESH':
            # Mesh objects can either be a reflecting surface or an emitter
            if ob.is_surface or ob.is_emitter:
                # Transform the vertex coordinates to world space
                ve = [list(ob.matrix_world * v.co) for v in ob.data.vertices]
                # In case of reflecting surfaces meshes need to be separated by material index
                mi_to_fa = {}
                mis = range(len(ob.data.materials))
                # Use the new tesselated faces api if available (2.63 and onwards)
                if hasattr(ob.data,'tessfaces'):
                    ob.data.update(calc_tessface=True)
                    faces = ob.data.tessfaces
                else: 
                    faces = ob.data.faces
                for f in faces:
                    mi = f.material_index
                    if mi in mis:
                        fvs = f.vertices
                        if len(fvs) == 4: vs = [[fvs[0],fvs[1],fvs[2]],[fvs[0],fvs[2],fvs[3]]]
                        else: vs = [fvs]
                        fa = mi_to_fa.get(mi,[])
                        for f in vs: fa.append([ve[i] for i in f])
                        mi_to_fa[mi] = fa
                    else:
                        print ("Warning no material assigned to slot %d for object %s"%(mi,ob.name))
                if ob.is_emitter:
                    # In case of an emitter material indices are not important so the dictionary is flattened
                    mesh_block = packblock('mesh',sum([fa for mi,fa in mi_to_fa.items()],[]))
                    block_id = '3src' if ob.is_tripleband else 'ssrc'
                    fn = '//%s.sine.wav'%(ob.name) if ob.is_sine else ob.filename
                    # The file is a computer generated sine, generate it and write to file
                    if ob.is_sine:
                        sine_args = [normpath(abspath(fn)),
                            ob.sine_attack,
                            ob.sine_attack_exponent,
                            ob.sine_decay,
                            ob.sine_decay_exponent]
                        if ob.sine_harmonics:
                            sine_args.extend((ob.sine_base_frequency,[(x.harmonic,x.amplitude) for x in ob.sine_frequencies]))
                            sines.write_compound_overtones(*sine_args)
                        else:
                            sine_args.append([(x.frequency,x.amplitude) for x in ob.sine_frequencies])
                            sines.write_compound(*sine_args)                            
                    block_args = [normpath(abspath(fn))]
                    if ob.is_tripleband:
                        block_args.extend((normpath(abspath(ob.filename2)),normpath(abspath(ob.filename3))))
                    block_args.extend((mesh_block,ob.gain))
                    writeblock(block_id,block_args)
                elif ob.is_surface:
                    for mi,fa in mi_to_fa.items():
                        writeblock('MESH',[ob.data.materials[mi].name]+fa)
        elif ob.type == 'EMPTY':
            if contains_animation:
                locs = []
                for fr in range(scn.frame_start,scn.frame_end+1,scn.frame_step):
                    scn.frame_set(fr)
                    locs.append(list(ob.location))
                loc = packblock('anim',locs)
            else: loc = list(ob.location)
            if ob.is_storyboard:
                generated_file,step_locs = storyboard.generate(ob,ob.step_size)
                if generated_file:
                    new_locs = []
                    step_index = 0
                    for fr in range(scn.frame_start,scn.frame_end+1,scn.frame_step):
                        scn.frame_set(fr)
                        step_loc = list(ob.location)
                        step_loc[2] = step_locs[step_index][2]
                        new_locs.append(step_loc)
                        step_index += 1
                    new_loc = packblock('anim',new_locs)
                    writeblock('SSRC',[normpath(generated_file),new_loc,ob.gain])
                    num_sounds += 1
            if ob.is_emitter:
                block_id = '3SRC' if ob.is_tripleband else 'SSRC'
                fn = '//%s.sine.wav'%(ob.name) if ob.is_sine else ob.filename
                if ob.is_sine:
                    sine_args = [normpath(abspath(fn)),
                        ob.sine_attack,
                        ob.sine_attack_exponent,
                        ob.sine_decay,
                        ob.sine_decay_exponent]
                    if ob.sine_harmonics:
                        sine_args.extend((ob.sine_base_frequency,[(x.harmonic,x.amplitude) for x in ob.sine_frequencies]))
                        sines.write_compound_overtones(*sine_args)
                    else:
                        sine_args.append([(x.frequency,x.amplitude) for x in ob.sine_frequencies])
                        sines.write_compound(*sine_args)                            
                block_args = [normpath(abspath(fn))]
                if ob.is_tripleband:
                    block_args.extend((normpath(abspath(ob.filename2)),normpath(abspath(ob.filename3))))
                block_args.extend((loc,ob.gain,ob.sound_offset))
                writeblock(block_id,block_args)
                num_sounds += 1
            if ob.is_listener:
                channels = 2 if ob.is_stereo else 1
                li = [normpath(abspath(ob.filename)),35.0,loc]
                if ob.is_stereo:
                    if contains_animation:
                        ears = []
                        for fr in range(scn.frame_start,scn.frame_end+1,scn.frame_step):
                            scn.frame_set(fr)
                            mat = ob.matrix_world.to_3x3()
                            vec = mat*mathutils.Vector((-1,0,0))
                            vec.normalize()
                            ears.append(list(vec))
                        ear = packblock('anim',ears)
                    else:
                        mat = ob.matrix_world.to_3x3()
                        vec = mat*mathutils.Vector((-1,0,0))
                        vec.normalize()
                        ear = list(vec)
                    li.append(ear)
                    li.append(ob.head_size)
                    li.append([ob.head_ab_low,ob.head_ab_mid,ob.head_ab_high])
                writeblock('OUT%d'%channels,li)
                num_recorders += 1
                recorder_names.append(ob.name)
    fs.close()
    
    scn.frame_set(old_frame)

def setup_exec_path():
    if run_test(): return
    exec_path = os.path.abspath(os.path.dirname(inspect.getfile(inspect.currentframe())))
    exec_path = os.path.join(exec_path,"ear")
    python_bits,alternative_bits = ('32','64') if sys.maxsize == 0x7fffffff else ('64','32')
    exec_ext = '.exe' if sys.platform.startswith('win') else ''
    if sys.platform.startswith('win'):
        exec_path = os.path.join(exec_path,"win")
    elif sys.platform == 'darwin':
        exec_path = os.path.join(exec_path,"osx")
    else:
        exec_path = os.path.join(exec_path,"linux")
    if os.path.exists(os.path.join(exec_path,python_bits)):
        exec_path = os.path.join(exec_path,python_bits)
    else:
        exec_path = os.path.join(exec_path,alternative_bits)
        print ("Trying %sbit binary, because %sbit is unavailable"%(alternative_bits,python_bits))
    exec_path = os.path.join(exec_path,"EAR%s"%exec_ext)
    if ( os.path.isfile(exec_path) and not os.access(exec_path, os.X_OK) ):
        # Let's just hope we have the permission to do so
        try:
            mode = os.stat(exec_path).st_mode
            mode |= stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH # = 0o111
            os.chmod(exec_path,mode)
        except: pass
    bpy.context.scene.ear_exec_path = exec_path
    if not run_test(): bpy.context.scene.ear_exec_path = ''
    
def get_exec_path():
    scn = bpy.context.scene
    path = normpath(abspath(scn.ear_exec_path))
    return path if os.path.isfile(path) and os.access(path, os.X_OK) else 'EAR'
    
def run_test():    
    return os.system('"%s" test'%get_exec_path()) == 0
    
def export_and_run(filename,exec_path='EAR'):
    export(filename)
    # Now start rendering the file
    if sys.platform.startswith('win'):
        os.system('start /LOW "E.A.R." "%s" render "%s"'%(exec_path,filename))
    else:
        xterm = '/usr/X11/bin/xterm' if sys.platform == 'darwin' else 'xterm'
        os.system('%s -e "%s" render "%s" &'%(xterm,exec_path,filename))

class EAR_OP_Export(bpy.types.Operator):
    bl_idname = "ear.export"
    bl_label = "Render with EAR"

    filepath = StringProperty(name="File Path", description="Filepath used for storing exported file", maxlen=1024, default="", subtype='FILE_PATH')
    filename_ext = ".ear"
    filter_glob = StringProperty(default="*.ear",options={'HIDDEN'})

    def execute(self, context):
        if not run_test():
            self.report({'ERROR'},'Unable to locate the EAR executable')
        else:
            export_and_run(self.filepath, get_exec_path())
        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.fileselect_add(self)
        return {'RUNNING_MODAL'}

class EAR_AddFreq(bpy.types.Operator):        
    bl_idname = "ear.addfreq"
    bl_label = "+"
    def execute(self,context):
        context.object.sine_frequencies.add()
        return {'FINISHED'}
        
class EAR_RemFreq(bpy.types.Operator):        
    bl_idname = "ear.remfreq"
    bl_label = "-"
    def execute(self,context):
        while True:
            removed_none = True
            i = 0
            for sf in context.object.sine_frequencies:
                if sf.remove: 
                    context.object.sine_frequencies.remove(i)
                    removed_none = False
                    break
                i += 1
            if removed_none: break
        return {'FINISHED'}

class EAR_Panel_Object(bpy.types.Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_label = "EAR: Evaluation of Acoustics using Ray-tracing"    
    
    def draw(self, context):
        layout = self.layout
        rd = context.scene
        ao = context.active_object
        if ao.type == "EMPTY":
            layout.prop(ao,'is_emitter','Emitter')
            if ao.is_emitter:
                layout.prop(ao,"gain","Gain")
                layout.prop(ao,"is_sine","Sine")
                if ao.is_sine:
                    row = layout.row(True)
                    row.prop(ao,"sine_attack","Attack")
                    row.prop(ao,"sine_attack_exponent","Attack Exponent")
                    row = layout.row(True)
                    row.prop(ao,"sine_decay","Decay")
                    row.prop(ao,"sine_decay_exponent","Decay Exponent")
                    layout.prop(ao,"sine_harmonics","Harmonics")
                    if ao.sine_harmonics:
                        layout.prop(ao,"sine_base_frequency","Base")
                    for sf in ao.sine_frequencies:
                        row = layout.row(True)
                        if ao.sine_harmonics:
                            row.prop(sf,"harmonic","Harmonic")
                        else:
                            row.prop(sf,"frequency","Frequency")
                        row.prop(sf,"amplitude","Amplitude")
                        row.prop(sf,"remove","-")
                    row = layout.row(True)
                    row.operator("EAR.addfreq","Add")
                    row.operator("EAR.remfreq","Remove")
                else:
                    layout.prop(ao,"is_tripleband","Triple band")
                    layout.prop(ao,"filename","")
                    if ao.is_tripleband:
                        layout.prop(ao,"filename2","")
                        layout.prop(ao,"filename3","")
            else:
                layout.prop(ao,'is_storyboard','Storyboard')
                if ao.is_storyboard:
                    layout.prop(ao,"step_size","Step size")
                    layout.prop(ao,"gain","Gain")
                layout.prop(ao,'is_listener','Listener')
                if ao.is_listener:
                    layout.prop(ao,'is_stereo','Stereo')
                    if ao.is_stereo:
                        row = layout.row()
                        row.prop(ao,"head_size","Head size")
                        row = layout.row(True)
                        row.label("Absorption")
                        row.prop(ao,"head_ab_low","Low")
                        row.prop(ao,"head_ab_mid","Mid")
                        row.prop(ao,"head_ab_high","High")
                    layout.prop(ao,"filename","")
        elif ao.type == "MESH":
            if not (ao.is_portal or ao.is_emitter):
                layout.prop(ao,'is_surface','Reflector')
            if not (ao.is_surface or ao.is_emitter):
                layout.prop(ao,'is_portal','Portal')
            if not (ao.is_portal or ao.is_surface):
                layout.prop(ao,'is_emitter','Emitter')
            if ao.is_portal or ao.is_emitter:
                layout.prop(ao,"filename","")
        if ao.is_emitter:
            layout.prop(ao,"sound_offset","Start time")
            
class EAR_Panel_Material(bpy.types.Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"
    bl_label = "EAR: Evaluation of Acoustics using Ray-tracing"
    
    def draw(self, context):
        layout = self.layout.row(True)
        rd = context.scene
        am = context.active_object.active_material
        if am:
            layout.prop(am,"aud_preset")
            if am.aud_preset == "0":
                
                layout = self.layout.row(True)
                
                layout.label("Reflectivity")
                layout.prop(am,"refl_low","Low")
                layout.prop(am,"refl_mid","Mid")
                layout.prop(am,"refl_high","High")
                
                layout = self.layout.row(True)
                
                layout.label("Transmittence")
                layout.prop(am,"refr_low","Low")
                layout.prop(am,"refr_mid","Mid")
                layout.prop(am,"refr_high","High")
                
                layout = self.layout.row(True)
                
                layout.label("Absorbtion")
                layout.label("Low:%.2f"%(1.0-am.refl_low-am.refr_low))
                layout.label("Mid:%.2f"%(1.0-am.refl_mid-am.refr_mid))
                layout.label("High:%.2f"%(1.0-am.refl_high-am.refr_high))
                
                layout = self.layout.row(True)
                
                layout.label("Specularity")
                layout.prop(am,"exp_low","Low")
                layout.prop(am,"exp_mid","Mid")
                layout.prop(am,"exp_high","High")
                
            layout = self.layout.row(True)
            layout.prop(am,"stepdir","Steps")
            
            
class EAR_Panel_Render(bpy.types.Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "render"
    bl_label = "EAR: Evaluation of Acoustics using Ray-tracing"
    
    def draw(self, context):
        rd = context.scene
        layout = self.layout.row(True)        
        layout.label("Absorption")
        layout.prop(rd,"ab_low","Low")
        layout.prop(rd,"ab_mid","Mid")
        layout.prop(rd,"ab_high","High")
        layout = self.layout.row(True)
        layout.label("Frequencies")
        layout.prop(rd,"freq1","Low")
        layout.prop(rd,"freq2","Mid")
        layout.prop(rd,"freq3","High")
        layout = self.layout.row()
        layout.prop(rd,"num_samples","Samples")
        layout = self.layout.row(True)
        layout.prop(rd,"drylevel","Dry level")
        layout = self.layout.row(True)
        layout.prop(rd,"debug_dir","Debug dir")
        layout = self.layout.row()
        layout.prop(rd,"max_threads","Threads")
        layout = self.layout.row()
        layout.prop(rd,"ear_exec_path","Executable")
        layout = self.layout.row(True)        
        layout.operator("EAR.export","Render audio","PLAY_AUDIO")
        
def draw(g,p,r,s,sel=False):
    p = p.copy()
    p.resize_4d()
    m = s.region_3d.perspective_matrix
    w,h = r.width,r.height
    v = m * p
    v /= v.w
    x,y = w/2+v.x*w/2,h/2+v.y*h/2.0-20.0
    if sel: bgl.glColor3f(1.0,0.6667,0.25)
    else: bgl.glColor3f(0.0,0.0,0.0)
    bgl.glBegin(bgl.GL_LINES)
    l = glyphs.glyphs.get(g,[])
    for p1,p2 in l:
        bgl.glVertex2f(p1[0]*3+x,p1[1]*3+y)
        bgl.glVertex2f(p2[0]*3+x,p2[1]*3+y)
    bgl.glEnd()
    
def draw_callback_px(c,r,s):
    for ob in c.scene.objects:
        # since ob.location gives incorrect results in case of a parented object for example
        if transpose_matrices:
            ob_location = mathutils.Vector([r[3] for r in ob.matrix_world[0:3]])
        else:
            ob_location = mathutils.Vector(ob.matrix_world[3][0:3])
        if ob.is_emitter: draw('loudspeaker',ob_location,r,s,ob.select)
        else:
            if ob.is_listener: draw('ear',ob_location,r,s,ob.select)
            # Arbitrarily offset the position of the feet downwards
            # Ideally perform a ray_cast here to determine the actual offset downwards
            if ob.is_storyboard: draw('feet',ob_location-mathutils.Vector((0,0,1.7)),r,s,ob.select)
            
region, handle = None, None

def register():
    bpy.utils.register_class(EAR_Prop_FrequencyComponent)
    bpy.utils.register_class(EAR_OP_Export)
    bpy.utils.register_class(EAR_Panel_Render)
    bpy.utils.register_class(EAR_Panel_Material)
    bpy.utils.register_class(EAR_Panel_Object)
    bpy.utils.register_class(EAR_AddFreq)
    bpy.utils.register_class(EAR_RemFreq)
    register_properties()
    global region,handle
    try:
        area = [a for a in bpy.data.window_managers[0].windows[0].screen.areas if a.type == 'VIEW_3D'][0]
        space = [r for r in area.spaces if r.type == 'VIEW_3D'][0]
        region = [r for r in area.regions if r.type == 'WINDOW'][0]
        handle = region.callback_add(draw_callback_px, (bpy.context,region,space), 'POST_PIXEL')
    except:
        print ("Failed to register draw callback")
    
def unregister():
    bpy.utils.unregister_class(EAR_Prop_FrequencyComponent)
    bpy.utils.unregister_class(EAR_OP_Export)
    bpy.utils.unregister_class(EAR_Panel_Render)
    bpy.utils.unregister_class(EAR_Panel_Material)
    bpy.utils.unregister_class(EAR_Panel_Object)
    bpy.utils.unregister_class(EAR_AddFreq)
    bpy.utils.unregister_class(EAR_RemFreq)
    global region,handle
    if handle: region.callback_remove(handle)
