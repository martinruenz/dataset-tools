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
    "--fps"
        help = "fps of the camera"
        required = true
    "--startstamp"
        help = ""
        required = true
    "--out"
        help = "path to the output log file"
        required = true
end
params = parse_args(s)
poses = readdlm(params["logfile"])
n = size(poses)[1]
fps = tryparse(Float64,params["fps"]);
incr = 1.0 / get(fps);
startstamp = get(tryparse(Float64,params["startstamp"]));

for i=1:n
  poses[i,1] = startstamp + (poses[i,1]-1)*incr
end

writedlm(params["out"], poses, " ");

print("done.")
return 0
