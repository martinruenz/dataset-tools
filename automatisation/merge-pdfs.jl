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

if length(ARGS) < 3  println("Not enough parameters."); exit(1); end
outPath = pop!(ARGS)
for p in ARGS if !isfile(p) println("Input file not found."); exit(1); end end
if isfile(outPath) println("Output exists."); exit(1); end

tmpPath = "$(tempname())-merging.pdf"
cp(ARGS[1], tmpPath)
for i = 2:length(ARGS)
  run(`pdftk $tmpPath multibackground $(ARGS[i]) output $outPath`)
  mv(outPath, tmpPath, remove_destination=true)
end
mv(tmpPath, outPath)
