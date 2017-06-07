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

using ArgParse

s = ArgParseSettings()
@add_arg_table s begin
    "--logfile"
        help = "path to your TUM-RGBD log file"
        required = true
    "--out"
        help = "path to the output ply file"
        required = true
end
params = parse_args(s)
poses = readdlm(params["logfile"])
n = size(poses)[1]

print("Processing $n poses...")

open(params["out"],"w") do f

  println(f, "ply")
  println(f, "format ascii 1.0")
  println(f, "element vertex $n")
  println(f, "property float x")
  println(f, "property float y")
  println(f, "property float z")
  println(f, "end_header")
  for i = 1:size(poses)[1]
      println(f, "$(poses[i,2]) $(poses[i,3]) $(poses[i,4])")
  end
end

print("done.")
return 0
