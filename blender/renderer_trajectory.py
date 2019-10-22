# ==================================================================
# This file is part of https://github.com/martinruenz/dataset-tools
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>
# ==================================================================

# blender --background --python viewpoint_renderer.py

import bpy
import math
import mathutils

path_mesh = "/media/martin/data/Datasets/External/ScanNet/scene0000_00_vh_clean_2.ply"
path_poses = "/media/martin/data/Datasets/External/ScanNet/scene0000_00-b.txt"
read_matrices = True


scene = bpy.context.scene

# Load mesh
bpy.ops.import_mesh.ply(filepath=path_mesh)

# Add camera
camera = bpy.data.cameras.new("Camera")
camera_object = bpy.data.objects.new("Camera", camera)
camera_object.rotation_mode = "QUATERNION"
bpy.context.scene.objects.link(camera_object)

# Poses
current_frame = 0
with open(path_poses, "r") as posefile:
    poses = posefile.readlines()
    print("Trajectory file has",len(poses),"poses")
    for l in poses:
        current_frame = current_frame+1
        scene.frame_set(current_frame)

        pose = l.split()
        pose = [float(x) for x in pose]

        t = (pose[1],pose[2],pose[3])
        if read_matrices:
            r = mathutils.Matrix()
            r[0][0:3], r[1][0:3], r[2][0:3] = pose[4:7], pose[7:10], pose[10:13]
            q = r.to_quaternion()
            #q.negate()
            #euler = r.to_euler()
        else:
            q = mathutils.Quaternion((pose[7],pose[4],pose[5],pose[6]))


        flip = mathutils.Quaternion(q*mathutils.Vector((1,0,0)), math.radians(180.0))
        q = flip * q

        camera_object.rotation_quaternion = q
        #camera_object.rotation_euler = euler
        camera_object.location = t
        camera_object.keyframe_insert(data_path="rotation_quaternion", index=-1)
        #camera_object.keyframe_insert(data_path="rotation_euler", index=-1)
        camera_object.keyframe_insert(data_path="location", index=-1)

        if(current_frame > 200):
            break

    print("Imported",current_frame,"frames")
