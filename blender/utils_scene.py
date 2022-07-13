import sys
import bpy
import os
from mathutils import Vector, Matrix, Quaternion
import numpy as np
from bpy_extras.image_utils import load_image
from pathlib import Path
import bmesh
from utils_material import *
from utils_common import *
from utils_datasets import load_transforms_csv
from geometry import *
import utils_mesh

# TODO make functions more minimal:
# - dont use transform_wc parameter and dont add to collection
# - instead return object and let user add to scene


def get_collection(collection):
    if collection is None:
        # Root collection when not specified (None)
        return bpy.data.scenes[0].collection
    if not collection in bpy.data.collections:
        # Create collection if it doesn't exist
        c = bpy.data.collections.new(collection)
        bpy.data.scenes[0].collection.children.link(c)
        return c
    return bpy.data.collections.get(collection)


def get_object_bb_corners(object):
    return np.stack(
        [np.array([c[0], c[1], c[2]]) for c in object.bound_box],
        axis=0
    )
    # (mathutils.Vector(b) for b in object.bound_box)
#
# def get_object_bb_center(object):
#     local_center = sum((mathutils.Vector(b) for b in object.bound_box), mathutils.Vector()) / 8.0


def set_environment_map(path_image):
    scene = bpy.context.scene
    world = scene.world
    world.use_nodes = True
    node_tree = world.node_tree
    environment_node = node_tree.nodes.new("ShaderNodeTexEnvironment")
    environment_node.image = bpy.data.images.load(path_image)
    node_tree.links.new(environment_node.outputs['Color'], node_tree.nodes['Background'].inputs['Color'])


def add_floor(h=1e4, w=1e4, collection=None, material=None, up='Z', **kwargs):
    if material is None:
        if 'scale' not in kwargs:
            kwargs['scale'] = h / 3
        material = get_material_checker(**kwargs)
    mesh_data = create_plane_meshdata(h, w, **kwargs)
    floor = bpy.data.objects.new("floor", mesh_data)
    floor.data.materials.append(material)
    get_collection(collection).objects.link(floor)
    return floor


def add_material(object, material, **kwargs):
    """
    material: None, tuple # todo: material
    """
    object.data.materials.append(get_material(material, **kwargs))


def remove_material(object, material_name):
    object.data.materials.pop(index=object.data.materials.find(material_name))


def remove_materials(object):
    object.data.materials.clear()


# TODO default parameters
def add_camera_frustum(transform_wc=None, u0=0.5, v0=0.5, fu=1, fv=1, w=1, h=1, scale=1, radius=0.01, path_image=None, collection=None, material=(1,1,0), apply_modifiers=False, flip_image_Y=False, auto_refresh=True, **kwargs):

    parent = bpy.data.objects.new("frame", None)
    parent.empty_display_type = 'ARROWS'
    objects_collection = get_collection(collection).objects
    objects_collection.link(parent)
    print(scale)

    if transform_wc is not None:
        parent.matrix_world = Matrix(transform_wc)

    xl = u0;
    xh = (w*fu + u0);
    yl = v0;
    yh = (h*fv + v0);

    # Mesh
    vertices = [
        (xl,yl,1),  (xh,yl,1),
        (xh,yh,1),  (xl,yh,1),
        (xl,yl,1),  (0,0,0),
        (xh,yl,1),  (0,0,0),
        (xl,yh,1),  (0,0,0),
        (xh,yh,1)
    ]
    N = len(vertices)
    edges = list(zip(np.arange(N-1), np.arange(N-1)+1))
    mesh = bpy.data.meshes.new('frustum')
    mesh.from_pydata(vertices, edges, [])
    frustum = bpy.data.objects.new(mesh.name, mesh)
    objects_collection.link(frustum)
    frustum.parent = parent

    # Faces / Skin
    skin_modifier = frustum.modifiers.new("skin", 'SKIN')
    skin_modifier.use_smooth_shade = True
    for v in frustum.data.skin_vertices[0].data:
        v.radius[0] = radius
        v.radius[1] = radius
    if apply_modifiers:
        bpy.context.view_layer.objects.active = frustum
        bpy.ops.object.modifier_apply(modifier=skin_modifier.name)

    # Material
    add_material(frustum, material)

    if path_image is not None:
        # Geometry
        vertices = [
            (xl,yl,1),  (xh,yl,1),
            (xh,yh,1),  (xl,yh,1)
        ]
        N = len(vertices)
        faces = [(0, 1, 2, 3)]
        edges = list(zip(np.arange(N-1), np.arange(N-1)+1))
        mesh = bpy.data.meshes.new('image_plane')
        mesh.from_pydata(vertices, edges, faces)
        mesh.uv_layers.new()
        mesh.uv_layers.active.data[0].uv = (0, 0 if flip_image_Y else 1)
        mesh.uv_layers.active.data[1].uv = (1, 0 if flip_image_Y else 1)
        mesh.uv_layers.active.data[2].uv = (1, 1 if flip_image_Y else 0)
        mesh.uv_layers.active.data[3].uv = (0, 1 if flip_image_Y else 0)

        frame_object = bpy.data.objects.new(mesh.name, mesh)
        frame_object.parent = parent
        objects_collection.link(frame_object)

        # Material
        material = bpy.data.materials.new(name="photo_material")
        material.use_nodes = True
        mat_nodes = material.node_tree.nodes
        mat_links = material.node_tree.links
        mat_nodes.clear()
        node_t = mat_nodes.new('ShaderNodeTexImage')
        if type(path_image) == list:
            assert len(path_image) > 0
            p_img = str(path_image[0])
        else:
            p_img = str(path_image)
        node_t.image = load_image(p_img, None, recursive=False)
        if type(path_image) == list:
            node_t.image.source = 'SEQUENCE'
            node_t.image_user.frame_duration = len(path_image)
            node_t.image_user.use_auto_refresh = auto_refresh
        node_t.extension = 'CLIP'
        node_t.location = (0, 0)
        node_om = mat_nodes.new('ShaderNodeOutputMaterial')
        node_om.location = (900, 0)
        mat_links.new(node_t.outputs['Color'], node_om.inputs['Surface'])
        frame_object.data.materials.append(material)

    parent.scale = (scale, scale, scale)
    return parent


def add_camera_frustum_from_K(transform_wc=None, K=None, w=1, h=1, scale=1, radius=0.01, **kwargs):
    if K is None:
        K = np.eye(4)
        K[:2, 2] = w/2, h/2
    Kinv = np.linalg.inv(K)
    return add_camera_frustum(transform_wc, Kinv[0,2], Kinv[1,2], Kinv[0,0], Kinv[1,1], w, h, scale, radius, **kwargs)


# TODO input 8 points to handle aabb and local box
def add_axis_aligned_bb(coord_min: np.array,
                        coord_max: np.array,
                        material=(0,1,0),
                        border_radius=0.01,
                        name='box',
                        collection=None):

    # Create object and link object to scene
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1)
    bmesh.ops.translate(bm, verts=bm.verts, vec=(0.5, 0.5, 0.5))
    bmesh.ops.scale(bm, verts=bm.verts, vec=(
        coord_max[0]-coord_min[0],
        coord_max[1]-coord_min[1],
        coord_max[2]-coord_min[2]
    ))
    for f in bm.faces:
        bm.faces.remove(f)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    cube = bpy.data.objects.new(name, mesh)
    cube.location = coord_min
    get_collection(collection).objects.link(cube)

    # Faces / Skin
    skin_modifier = cube.modifiers.new("skin", 'SKIN')
    skin_modifier.use_smooth_shade = True
    for v in cube.data.skin_vertices[0].data:
        v.radius[0] = border_radius
        v.radius[1] = border_radius

    subdivision = cube.modifiers.new("subsurf", 'SUBSURF')
    add_material(cube, material)

    return cube

def add_axis_aligned_bb_to_object(object, **kwargs):
    corners = get_object_bb_corners(object)
    a = np.min(corners, axis=0)
    b = np.max(corners, axis=0)
    box = add_axis_aligned_bb(a, b, **kwargs)
    box.parent = object


def add_pointcloud(points,
                   shape="sphere",
                   collection=None,
                   radius=0.01,
                   material=(0,1,0),
                   name=None,
                   apply_modifiers=False,
                   **kwargs):
    """
    points: np.array (Nx3) or list of points or path
    """

    # Get object name
    if name is None:
        if is_path_type(points):
            name = Path(points).stem
        else:
            name = 'cloud'

    # Return object if it already exists
    if name in bpy.data.objects:
        return bpy.data.objects[name]

    # Load points if path
    if is_path_type(points):
        points = utils_mesh.import_points(points)

    # Create object
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(points, [], [])
    cloud = bpy.data.objects.new(mesh.name, mesh)
    get_collection(collection).objects.link(cloud)

    # Faces / Skin
    skin_modifier = cloud.modifiers.new("skin", 'SKIN')
    skin_modifier.use_smooth_shade = True
    for v in cloud.data.skin_vertices[0].data:
        v.radius[0] = radius
        v.radius[1] = radius
    if apply_modifiers:
        bpy.context.view_layer.objects.active = cloud
        bpy.ops.object.modifier_apply(modifier=skin_modifier.name)


    # Subdivision modifier
    shape = shape.lower()
    if shape == 'sphere':
        subdivision = cloud.modifiers.new("subsurf", 'SUBSURF')
        subdivision.render_levels = 1
        if apply_modifiers:
            bpy.context.view_layer.objects.active = cloud
            bpy.ops.object.modifier_apply(modifier=subdivision.name)
    elif shape == 'cube':
        ... # nothing to do

    add_material(cloud, material)
    return cloud

def get_circle_vertices(radius=1, num_vertices=16, **kwargs):
    x = np.sin(np.linspace(0,2*np.pi,num_vertices+1)[:-1])
    y = np.cos(np.linspace(0,2*np.pi,num_vertices+1)[:-1])
    return np.stack([x, y, np.zeros(num_vertices)], axis=-1)    

def get_circle_mesh(circle_name='circle', **kwargs):
    vertices = get_circle_vertices(**kwargs)
    n = vertices.shape[0]
    edges = list(zip(np.arange(n), (np.arange(n)+1) % n))
    mesh = bpy.data.meshes.new(circle_name)
    mesh.from_pydata(vertices, edges, [])
    return mesh

def add_surfels(points: np.array, normals: np.array, radii=0.005, collection="Collection", material=(0,1,0), name='surfel', **kwargs):
    assert points.shape == normals.shape
    num_points = points.shape[0]
    base_vertices = get_circle_vertices(radius=1, **kwargs)
    res_circle = base_vertices.shape[0]
    base_normal = mathutils.Vector((0,0,1))
    vertices = []
    edges = []
    faces = []
    r = radii
    for i in range(num_points):
        if type(radii) != float and type(radii) != int:
            r = radii[i]
        surfel_normal = mathutils.Vector(normals[i])
        transform = np.asarray(rotation_from_vectors(base_normal, surfel_normal) @ mathutils.Matrix.Scale(r, 4))
        transform[:3, 3] = points[i]
        vertices.append(transform_points(transform, base_vertices))
        faces.append(np.arange(res_circle)+i*res_circle)
        edges += list(zip(np.arange(res_circle)+i*res_circle, ((np.arange(res_circle)+1) % res_circle)+i*res_circle))
    vertices = np.concatenate(vertices, axis=0)
    faces = [tuple(x) for x in np.stack(faces, axis=0)]
    mesh = bpy.data.meshes.new('test')
    mesh.from_pydata(vertices, edges, faces)
    surfels = bpy.data.objects.new(mesh.name, mesh)
    get_collection(collection).objects.link(surfels)

def add_trajectory(transforms_wc,
                   paths_images=None,
                   intrinsics=None,
                   freq_frust=1,
                   freq_dot=1,
                   frustum_radius=0.1,
                   frustum_scale=1,
                   frustum_material=(1,0,0),
                   frustum_indices=None,
                   dot_material=(0,0,0),
                   dot_radius=None,
                   min_distance_cameras=0,
                   always_include_last_frust=True,
                   add_camera_animation=False,
                   start_frame_animation=0,
                   **kwargs):
    if frustum_scale is None:
        positions = np.stack([np.array(t)[:3, 3] for t in transforms_wc])
        print(np.max(positions, axis=0))
        frustum_scale = np.linalg.norm(np.max(positions, axis=0) - np.min(positions, axis=0))
        frustum_scale *= 0.01
    dots = []
    last_transform_wc = None
    dot_material = get_material(dot_material)
    frustum_material = get_material(frustum_material)
    points = None
    frustum_animation = None
    frustums = []
    if add_camera_animation:
        K = intrinsics
        if type(intrinsics) == list:
            K = intrinsics[0]
        frustum_animation = add_camera_frustum_from_K(K=K, path_image=paths_images, material=frustum_material, scale=frustum_scale, radius=frustum_radius, **kwargs)
        if freq_frust == 1:
            freq_frust = 0
            always_include_last_frust = False
    if frustum_indices is not None and freq_frust == 1:
        freq_frust = 0
        always_include_last_frust = False
    for i, transform_wc in enumerate(transforms_wc):
        frust_freq_con = (freq_frust > 0) and (i % freq_frust == 0)
        frust_dist_con = (last_transform_wc is None) or (np.linalg.norm(last_transform_wc[:3,3]-transform_wc[:3,3]) >= min_distance_cameras)
        frust_last_con = always_include_last_frust and i==len(transforms_wc)-1
        frust_indi_con = frustum_indices is not None and i in frustum_indices
        if (frust_freq_con and frust_dist_con) or frust_last_con or frust_indi_con:
            # Add cam
            K = intrinsics
            if type(intrinsics) == list:
                K = intrinsics[i]
            path_image = paths_images[i] if type(paths_images) == list else None
            f = add_camera_frustum_from_K(transform_wc, K=K, path_image=path_image, material=frustum_material, scale=frustum_scale, radius=frustum_radius, **kwargs)
            frustums.append(f)
            last_transform_wc = transform_wc
        elif (freq_dot > 0) and (i % freq_dot == 0):
            # Add dots
            dots.append(np.array(transform_wc)[:3, 3])
        if add_camera_animation:
            frustum_animation.matrix_world = Matrix(transform_wc) @ Matrix.Scale(frustum_scale, 4)
            frustum_animation.keyframe_insert('location', index=-1, frame=start_frame_animation+i)
            frustum_animation.keyframe_insert('rotation_euler', index=-1, frame=start_frame_animation+i)
    if len(dots):
        points = add_pointcloud(np.stack(dots, axis=0), radius=0.06 * frustum_scale if dot_radius is None else dot_radius, material=dot_material, **kwargs)
    return points, frustums, frustum_animation


def add_trajectory_csv(*args, **kwargs):
    return add_trajectory(load_transforms_csv(*args, **kwargs), *args, **kwargs)


def add_lines(points_a: np.array,
              points_b: np.array,
              radius=0.1,
              collection=None,
              name='lines',
              material=(0,0,0),
              parent=None):

    N = points_a.shape[0]
    material = get_material(material)
    objects_collection = get_collection(collection).objects

    # HACK, otherwise (>1 line skinning) blender bug: https://developer.blender.org/T74943
    if N > 1:
        parent = bpy.data.objects.new(name + '_parent', None)
        objects_collection.link(parent)
        return [add_lines(points_a[i].reshape(1,3), points_b[i].reshape(1,3), radius, collection, name, material, parent)
                for i in range(N)]

    # Mesh
    assert points_a.shape[0] == points_b.shape[0]
    vertices = np.concatenate([points_a, points_b], axis=1).reshape(-1, 3)
    edges = [(a,b,) for (a,b) in np.arange(N*2).reshape(-1,2)]

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, edges, [])
    lines = bpy.data.objects.new(mesh.name, mesh)

    if parent is not None:
        lines.parent = parent
    objects_collection.link(lines)

    # Faces / Skin
    skin_modifier = lines.modifiers.new("skin", 'SKIN')
    skin_modifier.use_smooth_shade = True
    for v in lines.data.skin_vertices[0].data:
        v.radius[0] = radius
        v.radius[1] = radius

    subdivision = lines.modifiers.new("subsurf", 'SUBSURF')
    add_material(lines, material)

    return lines


def add_lines_csv(path, **kwargs):
    """
    Load lines from rows in csv file, where each row is <x1, y1, z1, x2, y2, z2>
    """
    p = np.loadtxt(path)
    pa, pb = p[:,:3], p[:,-3:]
    return add_lines(pa,pb, **kwargs)


def get_objects_in_hierachy(object, list_children=None):
    if list_children is None:
        list_children = []
    list_children.append(object)
    for o in object.children:
        get_objects_in_hierachy(o, list_children)
    return list_children


def animate_object_snapshots(objects, start_frame=0, frame_indices=None,
                             animate_show=True, animate_hide=True, offset=1,
                             recursive=True):
    print(frame_indices)
    if frame_indices is not None:
        assert len(frame_indices) == len(objects)
    else:
        frame_indices = range(start_frame, start_frame+len(objects))
    for frame_index, o_parent in zip(frame_indices, objects):
        if recursive:
            obs = get_objects_in_hierachy(o_parent)
        else:
            obs = [o_parent]
        for o in obs:
            # render
            if animate_show:
                o.hide_render = True
                o.keyframe_insert('hide_render', index=-1, frame=frame_index-offset)
                o.hide_render = False
                o.keyframe_insert('hide_render', index=-1, frame=frame_index)
            if animate_hide:
                o.hide_render = True
                o.keyframe_insert('hide_render', index=-1, frame=frame_index+offset)
            # viewport
            if animate_show:
                o.hide_viewport = True
                o.keyframe_insert('hide_viewport', index=-1, frame=frame_index-offset)
                o.hide_viewport = False
                o.keyframe_insert('hide_viewport', index=-1, frame=frame_index)
            if animate_hide:
                o.hide_viewport = True
                o.keyframe_insert('hide_viewport', index=-1, frame=frame_index+offset)


# === Examples ===

# Lines
#a = np.random.randn(15,3)
#b = np.random.randn(15,3)
#add_lines(a,b, radius=0.02)

# Lines from csv
# add_lines_csv('/home/ruenz/data/scannet/scannetv2/scene0427_00/bbs_0.txt', radius=0.007, material='red')


# Pointcloud
# add_pointcloud(np.random.randn(50,3))

# Trajectory
# add_trajectory_csv('/home/martin/Desktop/rgbd_dataset_freiburg2_desk-groundtruth.txt', freq_frust=160, freq_dot=40)

# Camera
# K = np.array([[500, 0, 320],[0,500,240],[0,0,1]])
# transform_wc = np.eye(4)
# transform_wc[0, 3] = 1
# add_camera_frustum_from_K(transform_wc, K, 640, 480, path_image='/home/martin/Pictures/threema-20190617-013803-219c3ea473d7aca63260258121856836740.jpg')
