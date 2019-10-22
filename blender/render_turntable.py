# Inspired by: https://github.com/panmari/stanford-shapenet-renderer.git
# blender --background --python mytest.py -- --views 10 /path/to/my.obj
import sys, argparse
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent.absolute()))
import utils_calib
import utils_pose
import utils_mesh
import bpy
import mathutils
import math
import random
import os

parser = argparse.ArgumentParser(description='This tool renders masks, depthmaps, ...')
parser.add_argument('input', type=str,
                    help='Path to the input mesh to be rendered.')
parser.add_argument('--num_views', type=int, default=-1,
                    help='number of views to be rendered')

parser.add_argument('--output', type=str, default='/tmp',
                    help='Path of the output directory')
# parser.add_argument('--scale', type=float, default=1,
                    # help='Scaling factor applied to model. Depends on size of mesh.')
# parser.add_argument('--remove_doubles', type=bool, default=True,
#                     help='Remove double vertices to improve mesh quality.')
# parser.add_argument('--edge_split', type=bool, default=True,
#                     help='Adds edge split filter.')
# parser.add_argument('--depth_scale', type=float, default=1.4,
                    # help='Scaling that is applied to depth. Depends on size of mesh. Try out various values until you get a good result. Ignored if format is OPEN_EXR.')
# parser.add_argument('--color_depth', type=str, default='8',
#                     help='Number of bit per channel used for output. Either 8 or 16.')
parser.add_argument('--depth_format', type=str, default='PNG',
                    help='Format of files generated. Either PNG or OPEN_EXR')

parser.add_argument('--calib', type=str, required=True,
                    help='Path to txt containing <w> <h> <fx> <fy> <cx> <cy>')

parser.add_argument('--distance', type=float, default=1,
                    help='Distance of the camera from the origin')

parser.add_argument('--elevation_angle', type=str, default='random',
                    help='Set elevation angle in degree (or "random")')

parser.add_argument('--render_checkerboard', action='store_true',
                    help='Also render the object with checkerboard pattern')


"""
Example calib file:
640 480 500 500 250 250\n
"""

print("Python version:", sys.version)

# Parse input
argv = sys.argv[sys.argv.index("--") + 1:]
args = parser.parse_args(argv)
path_poses_file = str(Path(args.output) / "poses.txt")
path_output = Path(args.output)
random_elevation = False
if args.elevation_angle == "random":
    elevation = random.uniform(-85, 85)
    random_elevation = True
else:
    elevation = float(args.elevation_angle)

azimuth_step = 360.0 / args.num_views
calib = Path(args.calib).read_text().splitlines()[0].split()
w, h, fx, fy, cx, cy = [float(x) for x in calib]



# Use nodes and create aliases
scene = bpy.context.scene
scene.use_nodes = True
# print(type(scene))
# print(type(node_tree))
nodes = scene.node_tree.nodes
links = scene.node_tree.links

## Clean
for o in bpy.data.objects:
    o.select_set(True)
bpy.ops.object.delete(use_global=True)
nodes.clear()

# Load mesh
utils_mesh.import_mesh(args.input)
object = bpy.context.selected_objects[0]
# object.active_material.emit = 1
object.pass_index = 1

# Get camera
camera = utils_calib.create_pinhole_camera(w, h, fx, fy, cx, cy)
camera.location = (args.distance, 0, 0)
origin = bpy.data.objects.new("Empty", None)
origin.location = (0, 0, 0)
origin.rotation_mode = 'XYZ'
origin.rotation_euler = (0, math.radians(elevation), 0)
scene.collection.objects.link(origin)
look_at_constraint = camera.constraints.new(type='TRACK_TO')
look_at_constraint.track_axis = 'TRACK_NEGATIVE_Z'
look_at_constraint.up_axis = 'UP_Y'
look_at_constraint.target = origin
camera.parent = origin

# Setup scene
scene.render.engine = 'CYCLES'
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
# scene.render.alpha_mode = 'TRANSPARENT'
scene.view_layers["View Layer"].use_pass_object_index = True
scene.view_layers["View Layer"].use_pass_normal = True
# scene.render.layers["RenderLayer"].use_pass_color = True
# scene.render.image_settings.file_format = args.format
# scene.render.image_settings.color_depth = args.color_depth

# Setup rendering of mask
node_rl = nodes.new(type='CompositorNodeRLayers')
node_rl.location = (0,0)
# for i, instance in enumerate(instances):

node_mask = nodes.new(type='CompositorNodeIDMask')
node_mask.label = 'ID filter'
# node_m.location = (200,200)
node_mask.index = object.pass_index
links.new(node_rl.outputs['IndexOB'], node_mask.inputs[0])

node_mask_output = nodes.new(type='CompositorNodeOutputFile')
# node_o.location = (400,i*200)
node_mask_output.label = 'Mask Output'
node_mask_output.base_path = ""
node_mask_output.format.color_mode = 'BW'
# instance.fout_node = node_mask_output
links.new(node_mask.outputs['Alpha'], node_mask_output.inputs[0])

# Setup rendering of normals
node_scale_normal = nodes.new(type="CompositorNodeMixRGB")
node_scale_normal.blend_type = 'MULTIPLY'
node_scale_normal.inputs[2].default_value = (0.5, 0.5, 0.5, 1)
links.new(node_rl.outputs['Normal'], node_scale_normal.inputs[1])

node_bias_normal = nodes.new(type="CompositorNodeMixRGB")
node_bias_normal.blend_type = 'ADD'
node_bias_normal.inputs[2].default_value = (0.5, 0.5, 0.5, 0)
links.new(node_scale_normal.outputs[0], node_bias_normal.inputs[1])

node_normal_output = nodes.new(type="CompositorNodeOutputFile")
node_normal_output.label = 'Normal Output'
node_normal_output.base_path = ""
links.new(node_bias_normal.outputs[0], node_normal_output.inputs[0])

# Setup rendering of normalized object coordinates
mat_object_coord = bpy.data.materials.new('object_coord_mat')
mat_object_coord.use_nodes = True
mat_object_coord_nodes = mat_object_coord.node_tree.nodes
mat_object_coord_links = mat_object_coord.node_tree.links
mat_object_coord_nodes.clear()

node_coord = mat_object_coord_nodes.new(type="ShaderNodeTexCoord")
node_scale_coord = mat_object_coord_nodes.new(type="ShaderNodeMixRGB")
node_scale_coord.blend_type = 'MULTIPLY'
node_scale_coord.inputs[2].default_value = (0.5, 0.5, 0.5, 1)
mat_object_coord_links.new(node_coord.outputs['Object'], node_scale_coord.inputs[1])

node_bias_coord = mat_object_coord_nodes.new(type="ShaderNodeMixRGB")
node_bias_coord.blend_type = 'ADD'
node_bias_coord.inputs[2].default_value = (0.5, 0.5, 0.5, 0)
mat_object_coord_links.new(node_scale_coord.outputs[0], node_bias_coord.inputs[1])

node_emission_coord = mat_object_coord_nodes.new(type="ShaderNodeEmission")
node_emission_coord.inputs[1].default_value = 1.0 # strength
mat_object_coord_links.new(node_bias_coord.outputs[0], node_emission_coord.inputs[0])

node_output_coord = mat_object_coord_nodes.new(type="ShaderNodeOutputMaterial")
mat_object_coord_links.new(node_emission_coord.outputs[0], node_output_coord.inputs[0])

object.data.materials.append(mat_object_coord)


# Setup rendering of checkerboard pattern
if args.render_checkerboard:
    mat_checker = bpy.data.materials.new('checkerboard_mat')
    mat_checker.use_nodes = True
    mat_checker_nodes = mat_checker.node_tree.nodes
    mat_checker_links = mat_checker.node_tree.links
    mat_checker_nodes.clear()

    node_uv_checker = mat_checker_nodes.new(type="ShaderNodeTexCoord")
    node_tex_checker = mat_checker_nodes.new(type="ShaderNodeTexChecker")
    node_tex_checker.inputs[1].default_value = (0.9, 0.9, 0.9, 1)
    node_tex_checker.inputs[2].default_value = (0.05, 0.05, 0.05, 1)
    node_diffuse_checker = mat_checker_nodes.new(type="ShaderNodeBsdfDiffuse")
    node_output_checker = mat_checker_nodes.new(type="ShaderNodeOutputMaterial")

    # mat_checker_links.new(node_uv_checker.outputs['UV'], node_tex_checker.inputs[0])
    mat_checker_links.new(node_tex_checker.outputs['Color'], node_diffuse_checker.inputs[0])
    mat_checker_links.new(node_diffuse_checker.outputs['BSDF'], node_output_checker.inputs[0])

    object.data.materials.append(mat_checker)

    view_layer_checker = scene.view_layers.new("checker layer")
    view_layer_checker.material_override = mat_checker
    node_rl_checker = nodes.new(type='CompositorNodeRLayers')
    node_rl_checker.layer = "checker layer"

    mat_checker_output = nodes.new(type="CompositorNodeOutputFile")
    mat_checker_output.label = 'Checker Output'
    mat_checker_output.base_path = ""
    links.new(node_rl_checker.outputs['Image'], mat_checker_output.inputs[0])

# node_normal_output = nodes.new(type="CompositorNodeOutputFile")
# node_normal_output.label = 'Normal Output'
# node_normal_output.base_path = ""
# links.new(node_bias_normal.outputs[0], node_normal_output.inputs[0])

# Setup rendering of depth
node_depth_output = nodes.new(type="CompositorNodeOutputFile")
node_depth_output.label = 'Depth Output'
node_depth_output.base_path = ""
if args.depth_format == 'OPEN_EXR':
  links.new(node_rl.outputs['Depth'], node_depth_output.inputs[0])
else:
  print("BETTER VALUES, CONFROM TUM")
  # Remap as other types can not represent the full range of depth.
  node_map_depth = nodes.new(type="CompositorNodeMapValue")
  # Size is chosen kind of arbitrarily, try out until you're satisfied with resulting depth node_map_depth.
  node_map_depth.offset = [-0.7]
  node_map_depth.size = [1.4]
  node_map_depth.use_min = True
  node_map_depth.min = [0]
  links.new(node_rl.outputs['Depth'], node_map_depth.inputs[0])
  links.new(node_map_depth.outputs[0], node_depth_output.inputs[0])

# Create file for writing poses
pose_file = open(path_poses_file, "w")
pose_file.write("# This file contains camera -> world transformations (camera poses)\n")
pose_file.write("# translation_x translation_y translation_z quaternion_x quaternion_y quaternion_z quaternion_w\n")

path_color = str(path_output / 'color_{0:03d}.png')
path_mask = str(path_output / 'mask_{0:03d}.png')
path_depth = str(path_output / 'depth_{0:03d}.png')
path_normal = str(path_output / 'normal_{0:03d}.png')
path_nocs = str(path_output / 'nocs_{0:03d}.png')
path_checker = str(path_output / 'checker_{0:03d}.png')

for i in range(0, args.num_views):
    print("Rotation {}, {}".format((azimuth_step * i), math.radians(azimuth_step * i)))

    transform_line = utils_pose.transformation_to_string(camera.matrix_world, 1, True, True)
    pose_file.write(transform_line)

    scene.render.filepath = path_color.format(i)
    node_depth_output.file_slots[0].path = path_depth.format(i)
    node_normal_output.file_slots[0].path = path_normal.format(i)
    node_mask_output.file_slots[0].path = path_mask.format(i)

    if args.render_checkerboard:
        mat_checker_output.file_slots[0].path = path_checker.format(i)


    # node_output_coord.file_slots[0].path = path_mask.format(i)
    # albedo_file_output.file_slots[0].path = scene.render.filepath + "_albedo.png"

    bpy.ops.render.render(write_still=True)

    #TODO is there really no better way?
    os.rename(path_depth.format(i) + '0001.png', path_depth.format(i))
    os.rename(path_normal.format(i) + '0001.png', path_normal.format(i))
    os.rename(path_mask.format(i) + '0001.png', path_mask.format(i))

    if args.render_checkerboard:
        os.rename(path_checker.format(i) + '0001.png', path_checker.format(i))

    origin.rotation_euler[2] += math.radians(azimuth_step)
    if random_elevation:
        origin.rotation_euler[1] += math.radians(random.uniform(-85, 85))

pose_file.close()
