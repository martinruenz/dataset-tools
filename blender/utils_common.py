import os
import mathutils
import numpy as np


def is_path_type(t):
    return type(t) is str or issubclass(type(t), os.PathLike)


def normalized(v):
    return v / np.linalg.norm(v)


def matrix_from_tq(t, q):
    m = q.to_matrix().to_4x4()
    m.translation = t
    return m


def matrix_from_lookat(camera_position, look_at, up=None):
    if up is None:
        up = np.array([0.0, 0.0, 1.0])
    else:
        up = np.asarray(up)
    camera_position = np.asarray(camera_position)
    look_at = np.asarray(look_at)
    z = normalized(look_at - camera_position)
    x = normalized(np.cross(z, up))
    y = normalized(np.cross(z, x))
    m = np.eye(4, 4)
    m[:3, 0] = x
    m[:3, 1] = y
    m[:3, 2] = z
    m[:3, 3] = camera_position
    return m


def rotation_from_vectors(vector_from: mathutils.Vector, vector_to: mathutils.Vector):
    axis = vector_from.cross(vector_to)
    axis.normalize()
    angle = vector_from.angle(vector_to)
    return mathutils.Matrix.Rotation(angle, 4, axis)


def transform_points(matrix: np.array, points: np.array):
    assert points.shape[1] == 3
    rs = matrix[:3,:3].T
    t = matrix[:3, 3]
    return points @ rs + t.reshape(1,3)


