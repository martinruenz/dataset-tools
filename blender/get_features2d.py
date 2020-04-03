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

import bpy
from bpy_extras.object_utils import world_to_camera_view
from math import pi
from mathutils import Euler


scene = bpy.context.scene
cam = scene.camera
mesh = bpy.data.objects['spraybottle'] # TODO change object here
# get depandency map
degp = bpy.context.evaluated_depsgraph_get()

fg_file = open(bpy.path.abspath("//object_features_2d.txt"),'w')
poses_file = open(bpy.path.abspath("//poses.txt"), 'w')

for f in range(bpy.context.scene.frame_start, bpy.context.scene.frame_end):

  scene.frame_set(f)

  ID = 0
  num_visible = 0
  num_frustrum = 0
  num_bad = 0

  # object pose Tow
  T_ow = mesh.matrix_world.inverted()
  # camera pose under blender convention (z pointing backward)
  R_wc_bpy = cam.matrix_world.to_3x3()
  R_bpy_to_slam = Euler((pi, 0, 0), "XYZ").to_matrix()
  R_wc_slam = R_wc_bpy @ R_bpy_to_slam
  T_wc_slam = R_wc_slam.to_4x4()
  t_wc = cam.matrix_world.to_translation()
  T_wc_slam.translation = t_wc
  
  q_wo = mesh.matrix_world.to_quaternion()
  t_wo = mesh.matrix_world.to_quaternion()
  q_wc = T_wc_slam.to_quaternion()
  poses_file.write("%f %f %f %f %f %f %f " % (t_wc.x, t_wc.y, t_wc.z, q_wc.x, q_wc.y, q_wc.z, q_wc.w))
  poses_file.write("%f %f %f %f %f %f %f " % (t_wo.x, t_wo.y, t_wo.z, q_wo.x, q_wo.y, q_wo.z, q_wo.w))
  poses_file.write("\n")
  
  # evaluate depandency map
  particle_systems = mesh.evaluated_get(degp).particle_systems
  
  # camera position
  cam_t = cam.matrix_world.to_translation()
  cam_tm = T_ow @ cam_t

  for p in particle_systems[0].particles:
    ID = ID + 1

    # Check if point falls into cam frustrum
    proj = world_to_camera_view(scene, cam, p.location)
    if not (0.0 < proj.x < 1.0 and 0.0 < proj.y < 1.0 and cam.data.clip_start < proj.z <  cam.data.clip_end):
      continue
    num_frustrum = num_frustrum + 1

    # Intersect
    # feature coordinate under object coordinate frame
    feature_tm = T_ow @ p.location
    d = feature_tm - cam_tm
    d.normalize()
    hit, intersection_tm, intersection_n, intersection_index = mesh.ray_cast(cam_tm, d)

    if not hit:
      num_bad = num_bad + 1

    # Check occlusion
    dist = (intersection_tm - feature_tm).length
    if dist > 0.0005:
      continue
    num_visible = num_visible + 1

    # Get pixel coordinate
    render_scale = scene.render.resolution_percentage / 100.0
    px = int(proj.x * scene.render.resolution_x * render_scale)
    py = int(scene.render.resolution_y * render_scale) - int(proj.y * scene.render.resolution_y * render_scale)

    fg_file.write('%d %d %d ' %(ID, px, py))

  print("There were ", num_frustrum, " features inside the frustrum, of which ",
          num_visible, " were visible in frame (",num_bad," bad casts) ...")

  fg_file.write('\n')

fg_file.close()
poses_file.close()
