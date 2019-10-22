#!/usr/bin/julia

#==================================================================
This file is part of https://github.com/martinruenz/dataset-tools

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
==================================================================#

if size(ARGS)[1] < 1
  println("Please specify data with first parameter.")
  exit()
end

using Quaternions
using PyPlot

fig = plt[:figure](figsize=(15,15))
ax = fig[:add_subplot](111, projection="3d")
ax[:set_xlabel]("X axis")
ax[:set_ylabel]("Y axis")
ax[:set_zlabel]("Z axis")

function draw_axis(c,v,color)
  p = vec(c)+vec(v)
  ax[:plot]([c[1], p[1]], [c[2], p[2]], [c[3], p[3]], color=color, lw=3)
end

for file in ARGS
  data = readdlm(file)
  Q = mapslices(r -> Quaternion(r[8],r[5],r[6],r[7]), data, [2])
  for i = 1:size(Q)[1]
    R = rotationmatrix(Q[i])
    s = 0.01
    draw_axis(data[i,2:4], R * [s;0.0;0.0], "red")
    draw_axis(data[i,2:4], R * [0.0;s;0.0], "green")
    draw_axis(data[i,2:4], R * [0.0;0.0;s], "blue")
  end
end
plt[:axis]("equal")
plt[:show]()
print("Hit <enter> to exit")
readline()
