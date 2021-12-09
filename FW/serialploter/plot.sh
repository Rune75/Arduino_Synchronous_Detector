#!/bin/sh
#

killall tail
killall gnuplot_qt
killall grep

serialDev="/dev/ttyUSB0"
speed="115200"

echo "
set datafile separator ','
set xrange [0:500]
set yrange [200:1000]
set ylabel 'Detection [LSB]'
set y2range [40000:60000]
set y2tics 0,10000
set ytics nomirror
set y2label 'Ambient and Exited [LSB]'
set grid
#plot '< tail -n 200 data.txt' using 0:1 with lines axes x1y1 title 'Detection', '< tail -n 200 data.txt' using 0:2 with lines axes x1y2 title 'Ambient'
plot '< tail -n 500 data.txt' using 0:3 with lines axes x1y1 title 'Detected', '< tail -n 500 data.txt' using 0:2 with lines axes x1y2 title 'Exited', '< tail -n 500 data.txt' using 0:1 with lines axes x1y2 title 'Ambient'
reread" > liveplot.gnu


stty -F $serialDev $speed -raw -hup
echo -n "" > data.txt #file for data

gnuplot liveplot.gnu &

#cat $serialDev |grep --line-buffered -v "^$" > data
# solution for avoiding extra empty line feeds between data found here
# https://ubuntuforums.org/showthread.php?t=1674943

grep --line-buffered -v "^$" $serialDev > data.txt

