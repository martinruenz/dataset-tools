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
    "--gt-poses"
        help = "directory containing ground-truth poses"
        required = true
    "--ex-poses"
        help = "directory containing exported poses"
        required = true
    "--gt-masks"
        help = "directory containing ground-truth masks"
        required = true
    "--ex-masks"
        help = "directory containing exported masks"
        required = true
    "--out"
        help = "directory where output data is written to"
        required = true
end
DIRS = parse_args(s)

FILE_CONCAT = joinpath(DIRS["out"], "concat.txt")
FILE_MAP = joinpath(DIRS["out"], "mapping.txt")
FILE_IOU_PLOT = joinpath(DIRS["out"], "iou.svg")
FILE_CONV_TRAJ = joinpath(DIRS["out"], "pose-converted")
FILE_CONV_PLOT = joinpath(DIRS["out"], "trajectory")

searchdir(path,key) = filter(x->contains(x,key), readdir(path))

# Search a directory for trajectory files and create a ID => filename dict.
function loadTrajectories(path)
  r = Dict{Integer,String}()
  foreach(
    function (p)
      if countlines(joinpath(path, p)) < 2
        warn("Ignoring the file $p, because it contains less than two poses.")
        return
      end
      m = match(r"(\d+)", p)
      if m == nothing
        warn("Ignoring the file $p, because the filename does not contain the label ID.")
      else
        r[parse(Int32,String(m.captures[1]))] = p
      end
    end,
    searchdir(path, ".txt"))
  return r
end

# Get file names of trajectory *.txts
gtTrajectories = loadTrajectories(DIRS["gt-poses"])
exTrajectories = loadTrajectories(DIRS["ex-poses"])
allTrajectoryPaths = String[]

# Collect the paths of all trajectory files
sizehint!(allTrajectoryPaths, length(gtTrajectories) + length(exTrajectories))
for p in values(gtTrajectories) push!(allTrajectoryPaths, joinpath(DIRS["gt-poses"], p)) end
for p in values(exTrajectories) push!(allTrajectoryPaths, joinpath(DIRS["ex-poses"], p)) end

println("Found $(length(gtTrajectories)) ground-truth trajectories")
println("Found $(length(exTrajectories)) exported trajectories")

# Find the trajectory dimensions
xrange = []
yrange = []
zrange = []
for p in allTrajectoryPaths
  traj = readdlm(p)
  xrange = collect(extrema([traj[:,2]; xrange]))
  yrange = collect(extrema([traj[:,3]; yrange]))
  zrange = collect(extrema([traj[:,4]; zrange]))
end
println("x-range of trajectories: $xrange")
println("y-range of trajectories: $yrange")
println("z-range of trajectories: $zrange")

function parseFloat(strings)
  map(x->(
    v = tryparse(Float64,x);
    isnull(v) ? throw("Could not parse a part. Something is wrong with the data.")
              : get(v)),
    strings)
end

# Maps: exID => (gtID, [total, best-frame#, best-frame i-o-u])
labelMapping = Dict{Int32, Pair{Int32, Array{Float64, 1}}}()

# Intersection-over-union first
# if(!isfile(FILE_CONCAT)) TODO: find new solution
  println("Evaluating segmentation quality now...")
  mkpath(dirname(FILE_CONCAT))
  iouFile = open(FILE_CONCAT, "a")
  # Handle all labels in result
  for idEX in keys(exTrajectories)
    # Compute intersection-over-union for each gt-label, in order to find the best one
    println("Comparing label $idEX with ground-truth labels...")
    labelIou = Array(Float64,(length(gtTrajectories),3))
    row2ID = Array(Int32, length(gtTrajectories))
    j = 1
    for idGT in keys(gtTrajectories)
      println("==> GT-label $idGT")
      iou = split(readstring(pipeline(
        `evaluate_segmentation
        	--dir $(DIRS["ex-masks"])
        	--dirgt $(DIRS["gt-masks"])
        	--prefixgt Mask
        	--prefix Segmentation
        	--width 1
        	--widthgt 4
        	--starti 2
        	--labelgt $idGT
        	--label $idEX
          --outtxt $(DIRS["out"])/$idEX-$idGT.txt
          -v`,
        `./eval-iou.awk`
        )))
      # [5] total, [12] best frame #, [16] best frame i-o-u
      labelIou[j,:] = parseFloat([iou[5] iou[12] iou[16]])
      row2ID[j] = idGT
      println(labelIou)
      j += 1
    end
    jbest = indmax(labelIou[:,1])
    fbest = Int(labelIou[jbest,2])
    labelMapping[idEX] = Pair(row2ID[jbest], labelIou[jbest,:])
    lines = readlines("$(DIRS["out"])/$idEX-$(row2ID[jbest]).txt")
    write(iouFile, "# Label $idEX and GT-label $(row2ID[jbest])\n")
    write(iouFile, "\"Label $idEX\"\n")
    write(iouFile, lines)
    write(iouFile, "\n\n")
    println("Best fit for label $idEX is $(row2ID[jbest]): Overall intersection-over-union is $(labelIou[jbest,1]) with best frame at $fbest.")
  end
  close(iouFile)
# TODO: find new solution
#   writedlm(FILE_MAP, labelMapping) #TODO Not readable by readdlm ;-/
# else
#   println("File $FILE_CONCAT already exists. Skipping computations.")
#   labelMapping = readdlm(FILE_MAP)
# end

function plotTraj(out,gt,ex,hideLegend=false)
  const border = 0.1
  xborder = border * norm(xrange)
  yborder = border * norm(yrange)
  println("Plotting error between:")
  println(gt)
  println(ex)
  run(`python ../external/TUM-RGBD/evaluate_ate.py
              --scale 1 --verbose
              $(hideLegend ? "--trajectories_only" : [])
              --xrange $(xrange[1]-xborder) $(xrange[2]+xborder)
              --yrange $(yrange[1]-yborder) $(yrange[2]+yborder)
              --plot $out
              $gt $ex`)
end

gtcameraFile = joinpath(DIRS["gt-poses"], gtTrajectories[0])
cameraFile = joinpath(DIRS["ex-poses"], exTrajectories[0])
plotPDFs = ["$FILE_CONV_PLOT-0.pdf"]
plotTraj(plotPDFs[1], gtcameraFile, cameraFile)
for idEX in keys(exTrajectories)
  if idEX == 0
    # Background/Camera is special case
    continue
  end
  idGT = labelMapping[idEX][1]
  m = labelMapping[idEX][2]
  objectFile = joinpath(DIRS["ex-poses"], exTrajectories[idEX])
  gtobjectFile = joinpath(DIRS["gt-poses"], gtTrajectories[idGT])
  outobjectFile = "$FILE_CONV_TRAJ-$idEX.txt"
  v = readstring(`convert_poses
        --frame $(Int(m[2]))
        --object $objectFile --camera $cameraFile
        --gtobject $gtobjectFile --gtcamera $gtcameraFile
        --out $outobjectFile`)

  plotTraj("$FILE_CONV_PLOT-$idEX.pdf", gtobjectFile, outobjectFile, true)
  push!(plotPDFs, "$FILE_CONV_PLOT-$idEX.pdf")
end
rm("$FILE_CONV_PLOT-merged.pdf", force=true)
run(`merge-pdfs.jl $(plotPDFs) $FILE_CONV_PLOT-merged.pdf`)

readstring(`gnuplot -e "datafile='$FILE_CONCAT'; outfile='$FILE_IOU_PLOT'" eval-iou.gp`)
