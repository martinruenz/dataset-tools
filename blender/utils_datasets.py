import numpy as np
from pathlib import Path
import utils_scene
from utils_common import *
from mathutils import Vector, Matrix, Quaternion


def scannet_load_transforms_wc(path_sequence):
    path_sequence = Path(path_sequence)
    i = 0
    transforms_wc = []
    valid_indices = []
    while True:
        path_transform = path_sequence / 'pose' / (str(i) + '.txt')
        if path_transform.exists():
            transforms_wc.append(np.loadtxt(path_transform))
            valid_indices.append(i)
        else:
            break
        i += 1
    return transforms_wc, valid_indices


def scannet_load_cameras(path_sequence, with_images=True, **kwargs):
    path_sequence = Path(path_sequence)
    transforms_wc, indices = scannet_load_transforms_wc(path_sequence)
    path_images = None
    if with_images:
        path_images = [path_sequence / 'color' / (str(i)+'.jpg')
                       for i in indices]
    if 'intrinsics' not in kwargs:
        path_intrinsics = path_sequence / 'intrinsic' / 'intrinsic_color.txt'
        if path_intrinsics.exists():
            kwargs['intrinsics'] = np.loadtxt(path_intrinsics)[:3,:3]
            print(kwargs['intrinsics'])
        else:
            kwargs['intrinsics'] = np.array([[1000, 0, 500],
                                             [0, 1000, 500],
                                             [0, 0, 1]])
    utils_scene.add_trajectory(transforms_wc=transforms_wc,
                               paths_images=path_images,
                               w=1296, h=960,
                               **kwargs)


def load_transforms_csv(path_trajectory_csv,
                       columns={'tx': 1, 'ty': 2, 'tz': 3,
                                'qx': 4, 'qy': 5, 'qz': 6, 'qw': 7},
                       comment_char='#',
                       split_char=' ',
                       invert=False,
                       **kwargs):
    """
    Load list of 4x4 transformations from corresponding rows of a csv file.
    :param path_trajectory_csv: Path to csv file.
    :param columns: Define column indices of translation and quaternion components.
    :param comment_char: Set character inducing comments in csv file.
    :param split_char: Set character separating fields in csv file.
    :param invert: Set true to invert transformations.
    :return: List of 4x4 matrices.
    """
    transforms = []
    with open(path_trajectory_csv) as f:
        for l in f.readlines():
            if len(l) == 0:
                continue
            if l[0] == comment_char:
                continue
            cols = l.rstrip().split(split_char)
            cols = {k: float(cols[v]) for k, v in columns.items()}
            if 'qx' in columns:
                q = Quaternion((cols['qw'], cols['qx'], cols['qy'], cols['qz']))
                t = Vector((cols['tx'], cols['ty'], cols['tz']))
                m = matrix_from_tq(t, q)
                if invert:
                    m.invert()
                transforms.append(np.asarray(m))
            else:
                raise NotImplementedError()
    return transforms