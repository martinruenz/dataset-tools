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

scene = bpy.context.scene
cam = scene.camera
mesh = bpy.data.objects['blok_6'] # TODO change object here

tom = mesh.matrix_world.inverted()

file=open(bpy.path.abspath("//tracks.txt"),'w')

for f in range(bpy.context.scene.frame_start, bpy.context.scene.frame_end):

  scene.frame_set(f)

  ID = 0
  num_visible = 0
  num_frustrum = 0
  num_bad = 0

  cam_t = cam.matrix_world.to_translation()
  cam_tm = tom * cam_t

  for p in mesh.particle_systems[0].particles:
    ID = ID + 1

    # Check if point falls into cam frustrum
    proj = world_to_camera_view(scene, cam, p.location)
    if not (0.0 < proj.x < 1.0 and 0.0 < proj.y < 1.0 and cam.data.clip_start < proj.z <  cam.data.clip_end):
      continue
    num_frustrum = num_frustrum + 1

    # Intersect
    feature_tm = tom * p.location
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

    file.write('%d %d %d ' %(ID, px, py))

  print("There were ", num_frustrum, " features inside the frustrum, of which ",
          num_visible, " were visible in frame (",num_bad," bad casts) ...")

  file.write('\n')

file.close()
