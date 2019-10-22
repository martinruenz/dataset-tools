# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

bl_info = {
    "name": "Export Poses",
    "author": "Martin Rünz", # inspired by 'export cameras & markers' (Campbell Barton) and 'export Clarises' (Ruchir Shah)
    # see: https://wiki.blender.org/index.php/Extensions:2.6/Py/Scripts/Import-Export/ClarisseCameraExport
    "version": (0, 3),
    "blender": (2, 6, 9),
    "api": 60995,
    "location": "File > Export > Export Poses",
    "description": "Export poses of selected object / camera",
    "warning": "",
    "wiki_url": "http://wiki.blender.org/index.php/Extensions:",
    "tracker_url": "http://projects.blender.org/tracker/index.php?func=detail&aid=",
    'category': 'Import-Export'}


import bpy
import bpy_extras
import mathutils
import math
from mathutils import *
from math import *
from bpy.props import *
from bpy_extras.io_utils import ExportHelper

def transformationToString(T, scaling_factor, useQuaternions, positiveZ=False):
    t = T.to_translation() * scaling_factor
    r = T.to_3x3()
    if positiveZ:
        r = r * mathutils.Euler((math.radians(180.0), 0.0, 0.0), 'XYZ').to_matrix()
    result = '%f %f %f' %(t[0], t[1], t[2])
    if useQuaternions:
        q = r.to_quaternion()
        result = result + ' %f %f %f %f\n' %(q.x, q.y, q.z, q.w)
    else:
        r = r.to_euler()
        result = result + ' %f %f %f\n' %(r[0], r[1], r[2])
    return result

def exportPoses(context, filepath, frame_start, frame_end, scaling_factor, useQuaternions, appendFrameNumber, cam_positive_z):

    obj=[]
    scene = bpy.context.scene
    obj = scene.objects.active

    if obj == None:
        print("No active object.")
        return

    print("Exporting: '{}', scaling factor: {}".format(obj.name, scaling_factor))

    # Export world coordinates of currently selected object / camera
    frame_range = range(frame_start, frame_end + 1)
    myFile=open(filepath,'w')
    for f in frame_range:
        scene.frame_set(f)
        line = transformationToString(obj.matrix_world, scaling_factor, useQuaternions, obj.type == 'CAMERA' and cam_positive_z)
        if appendFrameNumber:
            line = ('%i ' % f) + line
        myFile.write(line)
    myFile.close()

    return

class PoseExporter(bpy.types.Operator, ExportHelper):

    bl_idname = "export_animation.cameras"
    bl_label = "Export Settings"

    filename_ext = ".txt"
    filter_glob = StringProperty(default="*.txt", options={'HIDDEN'})

    scaling_factor = FloatProperty(name="Scaling factor",
            description="Multiply position vector by this factor",
            default=1, min=0.0001, max=100000)


    frame_start = IntProperty(name="Start Frame",
            description="Start frame for export",
            default=0, min=0, max=300000)
    frame_end = IntProperty(name="End Frame",
            description="End frame for export",
            default=250, min=1, max=300000)

    quaternion_representation = BoolProperty(name="Use quaternions", default=True)
    append_frame_number = BoolProperty(name="Append frame", default=True)
    cam_positive_z = BoolProperty(name="Positive Z (cam)", default=True)

    def execute(self, context):
        exportPoses(context,
                    self.filepath,
                    self.frame_start,
                    self.frame_end,
                    self.scaling_factor,
                    self.quaternion_representation,
                    self.append_frame_number,
                    self.cam_positive_z)
                    #, self.alsoRelativeToCamera
        return {'FINISHED'}

    def invoke(self, context, event):
        self.frame_start = context.scene.frame_start
        self.frame_end = context.scene.frame_end

        wm = context.window_manager
        wm.fileselect_add(self)
        return {'RUNNING_MODAL'}


def menu_export(self, context):
    import os
    default_path = os.path.splitext(bpy.data.filepath)[0] + ".txt"
    self.layout.operator(PoseExporter.bl_idname, text="Poses Export (.txt)").filepath = default_path


def register():
    bpy.types.INFO_MT_file_export.append(menu_export)
    bpy.utils.register_class(PoseExporter)

def unregister():
    bpy.types.INFO_MT_file_export.remove(menu_export)
    bpy.utils.unregister_class(PoseExporter)

if __name__ == "__main__":
    register()
