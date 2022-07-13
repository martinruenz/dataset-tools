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
import pathlib
import numpy as np


def import_mesh(path, name=None, collection=None):
    """
    Imports obj / ply files, and returns object. If an object with the same name
    already exists, the existing object is returned and nothing is imported.
    """
    from utils_scene import get_collection
    p = pathlib.Path(path)
    ext = p.suffix.lower()
    path = str(path)
    if name is None:
        name = p.stem
    if name in bpy.data.objects:
        o = bpy.data.objects[name]
    elif ext == ".obj":
        bpy.ops.import_scene.obj(filepath=path)
        o = bpy.context.view_layer.objects.active
    elif ext == ".ply":
        bpy.ops.import_mesh.ply(filepath=path)
        o = bpy.context.view_layer.objects.active
    else:
        raise RuntimeError("Unknown mesh format: ", path)
    o.name = name
    c = get_collection(collection)
    if name not in c.objects:
        c.objects.link(o)
    return o


def import_points(path):
    ext = pathlib.Path(path).suffix.lower()
    path = str(path)
    if ext == ".xyz":
        return np.loadtxt(path)
    elif ext == ".npy":
        return np.load(path)
    else:
        raise RuntimeError("Unknown cloud format: ", path)

# def import_auto(path, collection=None):
