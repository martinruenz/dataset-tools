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

import mathutils
import math

def transformation_to_string(T, scaling_factor, useQuaternions, positiveZ=False):
    t = T.to_translation() * scaling_factor
    r = T.to_3x3()
    if positiveZ:
        r = r @ mathutils.Euler((math.radians(180.0), 0.0, 0.0), 'XYZ').to_matrix()
    result = '%f %f %f' %(t[0], t[1], t[2])
    if useQuaternions:
        q = r.to_quaternion()
        result = result + ' %f %f %f %f\n' %(q.x, q.y, q.z, q.w)
    else:
        r = r.to_euler()
        result = result + ' %f %f %f\n' %(r[0], r[1], r[2])
    return result
