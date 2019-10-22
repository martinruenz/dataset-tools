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
import os
import math
import random
from mathutils import *

scene = bpy.context.scene
scene.use_nodes = True
tree = scene.node_tree
nodes = tree.nodes
links = tree.links

# Settings

## Input
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
model_dir = "/media/martin/data/Datasets/SecondHands/Models/Objects"
# Valid models:
#     "AllanKey5/AllanKey5.wrl",
#     "Brush/brush.wrl",
#     "Cutter/cutter.wrl",
#     "Flashlight/Flashlight.wrl",
#     "Ladder/Ladder.wrl",
#     "Mallet/mallet.wrl",
#     "PhillipsScrewdriver/phillips.wrl",
#     "Pliers/pliers.wrl",
#     "ScrewdriverCross/ScrewdriverCross.wrl",
#     "Slotted_Screwdriver/slotted_screwdriver.wrl",
#     "SprayBottle/SprayBottle.wrl",
#     "Wrench/Wrench.wrl"
models = [
    {"path":"Brush/brush.wrl",
     "class_id": 1,
     "rotation": [0,90,0]},
    {"path":"Cutter/cutter.wrl",
     "class_id": 2,
     "rotation": [90,0,0]},
    {"path":"PhillipsScrewdriver/phillips.wrl",
     "class_id": 3},
    {"path":"Pliers/pliers.wrl",
     "class_id": 4,
     "rotation": [90,0,0]},
    {"path":"Slotted_Screwdriver/slotted_screwdriver.wrl",
     "class_id": 3},
    {"path":"SprayBottle/SprayBottle.wrl",
     "class_id": 5}
]

output_dir = "/media/martin/data2/datasets/SecondHands_MaskRCNN/renderings/"
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

## Sampling
#mean_altitude = 0
min_altitude = -0.8 * math.pi / 2
max_altitude = 0.8 * math.pi / 2
min_distance = 0.2
max_distance = 2

sigma_roll = 0.07 * math.pi
sigma_pitch = 0.00001 * math.pi
sigma_yaw = 0.00001 * math.pi

num_views = 20000
max_num_objects = 3

## Camera
calibration = [640.0, 480.0, 528.0, 528.0, 320.0, 240.0]
w, h, fx, fy, cx, cy = calibration
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

if not os.path.isdir(output_dir):
    os.mkdir(output_dir)


class Instance:
    def __init__(self, object, class_id, file):
        self.object = object
        self.object.name = file
        self.class_id = class_id
        self.file = file
        self.fout_node = None


# Setup

## Clean
for o in bpy.data.objects:
    o.select = True
bpy.ops.object.delete(use_global=True)
nodes.clear()

## Scene
scene.render.filepath = os.path.join(output_dir, "overlays/frame")
scene.frame_start = 0
scene.frame_end = num_views-1
# scene.use_nodes = True
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.render.alpha_mode = 'TRANSPARENT'
scene.render.layers["RenderLayer"].use_pass_object_index = True
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
    raise NotImplementedError("..")

### Camera object
cam_data = bpy.data.cameras.new("camera_data")
cam_data.type = 'PERSP'
cam_data.lens_unit = 'MILLIMETERS'
cam_data.sensor_width = 100
cam_data.lens = fx * cam_data.sensor_width / w
cam_data.sensor_fit = 'HORIZONTAL'
# cam_data.sensor_height = 1 # auto
camera = bpy.data.objects.new("Camera", cam_data)
camera.rotation_euler = [math.pi / 2, 0, 0]
camera.location = Vector((0,0,0))
bpy.context.scene.objects.link(camera)

# Load meshes
instances = []
for mp in models:
    # Load file
    path = os.path.join(model_dir, mp["path"])
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.import_scene.x3d(filepath=path)
    if len(bpy.context.selected_objects) != 1:
        # File contained multiple objects => add code to group
        raise RuntimeError("Number of objects in file {} != 1".format(path))

    # Set object properties
    obj = bpy.context.selected_objects[0]
    obj.rotation_euler = [0,0,0]
    rot = mp.get("rotation")
    if rot is not None:
        obj.rotation_euler = rot
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)

    obj.active_material.emit = 1

    # Create instance and copies
    instances.append(Instance(obj, mp["class_id"], mp["path"]))
    for j in range(max_num_objects-1):
        obj_dup = obj.copy()
        scene.objects.link(obj_dup)
        instances.append(Instance(obj_dup, mp["class_id"], mp["path"]))

# Create instance indexes and store class_ids in file
# with open(os.path.join(output_dir, 'class_ids.txt'), 'w') as class_file:
#     for i, instance in enumerate(instances, start=1):
#         class_file.write('{} {}\n'.format(i, instance.class_id))

# Setup rendering
node_rl = nodes.new(type='CompositorNodeRLayers')
node_rl.location = (0,0)
for i, instance in enumerate(instances):
    instance.object.pass_index = i + 1
    node_m = nodes.new(type='CompositorNodeIDMask')
    node_m.location = (200,i*200)
    node_m.index = instance.object.pass_index
    links.new(node_rl.outputs['IndexOB'], node_m.inputs[0])

    node_o = nodes.new(type='CompositorNodeOutputFile')
    node_o.location = (400,i*200)
    node_o.base_path = os.path.join(output_dir, "masks_raw")
    node_o.format.color_mode = 'BW'
    node_o.file_slots[0].path = 'instance-{}-frame-'.format(i+1)
    instance.fout_node = node_o
    links.new(node_m.outputs['Alpha'], node_o.inputs[0])

# Generate views
# Each line describes a frame by a list of (<mask_path> <label>) tuples
info_file = open(os.path.join(output_dir, "frame_info.txt"), "w")
for i in range(num_views):

    # By default, hide instances
    for instance in instances:
        instance.object.hide_render = True
        instance.object.hide = True
        instance.fout_node.mute = True
        instance.object.keyframe_insert("hide_render", index=-1, frame=i)
        instance.object.keyframe_insert("hide", index=-1, frame=i)
        instance.fout_node.keyframe_insert("mute", index=-1, frame=i)

    # Select visible instances
    view_num_objects = random.randint(1, max_num_objects)
    view_instances = random.sample(instances, view_num_objects)
    for instance in view_instances:
        instance.object.hide_render = False
        instance.object.hide = False
        instance.fout_node.mute = False
        instance.object.keyframe_insert("hide_render", index=-1, frame=i)
        instance.object.keyframe_insert("hide", index=-1, frame=i)
        instance.fout_node.keyframe_insert("mute", index=-1, frame=i)
        info_file.write("{}{:04d} {} ".format(instance.fout_node.file_slots[0].path, i, instance.class_id))
    info_file.write("\n")

    # Position instances
    for instance in view_instances:
        # Position
        d = random.uniform(min_distance, max_distance)
        x_min = d * (-cx) / fx
        x_max = d * (w-cx) / fx
        y_min = d * (-cy) / fy
        y_max = d * (h - cy) / fy
        x = random.uniform(x_min, x_max)
        y = random.uniform(y_min, y_max)
        instance.object.location = [x,d,y]
        instance.object.keyframe_insert("location", index=-1, frame=i)

        # Rotation
        alt = random.uniform(min_altitude, max_altitude)
        azi = random.uniform(0, 2 * math.pi)
        instance.object.rotation_euler = [azi, 0, alt]
        instance.object.keyframe_insert("rotation_euler", index=-1, frame=i)

info_file.close()
