#!/usr/bin/awk -f

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

BEGIN {
	RS="\n\n";
	FS="\n";

  maxframe=-1
  maxval=-1
  total=0
}

{
  # Get frame
  split($1, FrameDesc, " ")
  gsub(/\./, "", FrameDesc[3])
  frame = FrameDesc[3]

  # Get label 1
  if(NF > 2){
    split($2, LabelDesc0, " ")
    split($3, LabelDesc1, " ")
    if((LabelDesc0[1] == "Label") && (LabelDesc1[1] == "Label") && (LabelDesc1[3] > maxval)){
      maxframe = frame
      maxval = LabelDesc1[3]
    }
    #print "Frame " frame " label " LabelDesc[2] " val: " LabelDesc[3]
  }

  # Iterate labels
  #for(i = 2; i <= NF; i++) {
  #  split($i, LabelDesc, " ")
  #  if(LabelDesc[1] == "Label"){
  #      #print LabelDesc[3];
  #  }
  #  print "Frame " frame " label " $i
  #}
}

/Overall/ {
  if(NF > 6){
    split($6, OverallDesc, " ")
    total = OverallDesc[3]
  }
}

END {
  print "The intersection-over-union sum is " total " while the best frame is at " maxframe " with i-o-u of " maxval
}
