#!/usr/bin/gnuplot
#
# Creates a version of a plot, which looks nice for inclusion on web pages
#
# original author: Hagen Wierstorf, see:http://www.gnuplotting.org/attractive-plots/

reset

# wxt
#set terminal wxt size 410,250 enhanced font 'Verdana,9' persist
# png
#set terminal pngcairo size 410,250 enhanced font 'Verdana,9'
#set output 'nice_web_plot.png'
# svg
set terminal svg size 410,250 fname 'Verdana, Helvetica, Arial, sans-serif' \
fsize '9' rounded dashed
set output outfile

# define axis
# remove border on top and right and set color to gray
set style line 11 lc rgb '#808080' lt 1
set border 3 back ls 11
set tics nomirror
# define grid
set style line 12 lc rgb '#808080' lt 0 lw 1
set grid back ls 12

# color definitions
set style line 1 lc rgb '#ff0000' pt 1 ps 1 lt 1 lw 1 # --- red
set style line 2 lc rgb '#0000ff' pt 6 ps 1 lt 1 lw 1 # --- blue
set style line 3 lc rgb '#000000' pt 6 ps 1 lt 1 lw 1 # --- black

set key bottom right

set xlabel 'frame'
set ylabel 'intersection-over-union'
set yrange [0:1]

IGNORE_BELOW=0.001
filter(y)=(y>=IGNORE_BELOW)?(y):(1/0)

set datafile separator '\t' # Otherwise empty entries are not handled correctly
stats datafile every ::2

plot for [IDX=0:STATS_blocks-1]	datafile \
     index IDX \
		 using 1:(filter($3)) \
		 title columnheader(1) \
		 with lines \
		 linestyle IDX+1
