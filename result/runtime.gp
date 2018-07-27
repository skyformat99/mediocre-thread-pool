reset
set style fill solid
set xlabel 'thread pool size'
set ylabel 'requests per second( * 10^3)'
set title 'throughput comparison'
set term png enhanced font 'Consolas,10'
set output 'result/runtime.png'

plot [:][:1500] \
	'result/output.txt' using 2:xtic(1) with histogram title 'condition-variable', \
	'result/output.txt' using 3:xtic(1) with histogram title 'half-duplex-pipe', \
	'result/output.txt' using 4:xtic(1) with histogram title 'two-stage-mutex', \
	'result/output.txt' using 5:xtic(1) with histogram title 'work-group', \
	'result/output.txt' using ($0-0):(900):2 with labels title '' textcolor lt 1, \
	'result/output.txt' using ($0-0):(970):3 with labels title '' textcolor lt 2, \
	'result/output.txt' using ($0-0):(1040):4 with labels title '' textcolor lt 3, \
	'result/output.txt' using ($0-0):(1110):5 with labels title '' textcolor lt 4, \
