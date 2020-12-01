set xrange [0:200]
set yrange [20000:50000]

plot '< tail -n 200 data' with lines
reread
