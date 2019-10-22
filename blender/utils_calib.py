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
# Inspired by: http://blender.stackexchange.com/questions/15102/what-is-blenders-camera-projection-matrix-model/38189
# Thanks to user rfabbri

import bpy
from mathutils import Matrix

def dump(obj):
    for attr in dir(obj):
        print("obj.%s = %s" % (attr, getattr(obj, attr)))

def get_calibration(camd):
    f = camd.lens
    scene = bpy.context.scene
    resX = scene.render.resolution_x
    resY = scene.render.resolution_y
    scale = 0.01 * scene.render.resolution_percentage
    sensorWidth = camd.sensor_width # in mm
    sensorHeight = camd.sensor_height # in mm
    pixel_aspect_ratio = scene.render.pixel_aspect_x / scene.render.pixel_aspect_y

    if camd.sensor_fit == 'AUTO':
      sensorHeight = sensorWidth

    if camd.sensor_fit == 'VERTICAL' or (camd.sensor_fit == 'AUTO' and resX * scene.render.pixel_aspect_x <= resY * scene.render.pixel_aspect_y):
        # fixed sensor height
        s_v = resY / sensorHeight
        s_u = s_v / pixel_aspect_ratio
        # print("VERTICAL")
    else:
        # fixed sensor width
        s_u = resX / sensorWidth
        s_v = pixel_aspect_ratio * s_u
        # print("HORIZONTAL")

    s_u *= scale
    s_v *= scale

    # Parameters of intrinsic calibration matrix K
    alpha_u = f * s_u
    alpha_v = f * s_v
    u_0 = resX*scale / 2
    v_0 = resY*scale / 2
    skew = 0 # ignore skew

    # Print camera data
    #dump(camd);

    K = Matrix(
        ((alpha_u, skew,    u_0),
        (    0  ,  alpha_v, v_0),
        (    0  ,    0,      1 )))
    return K

def set_calibration(w, h, fx, fy, cx, cy, scene, camera_data):
    scene.render.resolution_percentage = 100
    scene.render.resolution_x = w
    scene.render.resolution_y = h
    aspect = fx / fy
    scene.render.pixel_aspect_x = 1
    scene.render.pixel_aspect_y = 1
    if aspect >= 1:
        scene.render.pixel_aspect_x = aspect
    else:
        scene.render.pixel_aspect_y = aspect
    if w/2 != cx or h/2 != cy:
        raise NotImplementedError("...")

    camera_data.type = 'PERSP'
    camera_data.lens_unit = 'MILLIMETERS'
    camera_data.sensor_width = 100
    camera_data.lens = fx * camera_data.sensor_width / w
    camera_data.sensor_fit = 'HORIZONTAL'


def create_pinhole_camera(w, h, fx, fy, cx, cy, ):
    cam_data = bpy.data.cameras.new("camera_data")
    set_calibration(w, h, fx, fy, cx, cy, bpy.context.scene, cam_data)
    camera = bpy.data.objects.new("camera", cam_data)
    bpy.context.scene.collection.objects.link(camera)
    bpy.context.scene.camera = camera
    return camera

if __name__ == "__main__":
    # Example of setting calibration:
    calibration = [640.0, 480.0, 528.0, 528.0, 320.0, 240.0]
    w, h, fx, fy, cx, cy = calibration
    K = get_calibration(bpy.data.objects['Camera'].data)
    print("previous calibration:\n", K)
    set_calibration(w,h,fx,fy,cx,cy,bpy.context.scene, bpy.data.cameras['Camera.001'])

    K = get_calibration(bpy.data.objects['Camera'].data)
    print("calibration:\n", K)
