# Inspired by: http://blender.stackexchange.com/questions/15102/what-is-blenders-camera-projection-matrix-model/38189
# Thanks to user rfabbri

import bpy
from mathutils import Matrix

def dump(obj):
    for attr in dir(obj):
        print("obj.%s = %s" % (attr, getattr(obj, attr)))

def getCalibration(camd):
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
        print("VERTICAL")
    else:
        # fixed sensor width
        s_u = resX / sensorWidth
        s_v = pixel_aspect_ratio * s_u
        print("HORIZONTAL")

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

if __name__ == "__main__":
    K = getCalibration(bpy.data.objects['Camera'].data)
    print(K)
