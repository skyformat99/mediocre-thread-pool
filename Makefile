CC = gcc
FLAGS = -Wall -std=gnu11 -O2
LIBRARY = -lpthread
EXEC =	condition-variable/thread-pool													\
	half-duplex-pipe/thread-pool													\
	two-stage-mutex/thread-pool													\
	work-group/thread-pool

all: $(EXEC)

condition-variable/thread-pool: main.c condition-variable/*.[ch]
	$(CC) $(FLAGS) -DIMPL="\"$@.h\"" $^ -o $@ $(LIBRARY)

half-duplex-pipe/thread-pool: main.c half-duplex-pipe/*.[ch]
	$(CC) $(FLAGS) -DIMPL="\"$@.h\"" $^ -o $@ $(LIBRARY)

two-stage-mutex/thread-pool: main.c two-stage-mutex/*.[ch]
	$(CC) $(FLAGS) -DIMPL="\"$@.h\"" $^ -o $@ $(LIBRARY)

work-group/thread-pool: main.c work-group/*.[ch]
	$(CC) $(FLAGS) -DIMPL="\"$@.h\"" $^ -o $@ $(LIBRARY)

run: $(EXEC)
	sudo perf stat --repeat 10 work-group/./thread-pool 4096
	rm -f result/*.txt

throughput-test: $(EXEC)
	for workers in 128 256 512 1024 2048 4096; do										\
		echo -n $$workers >> result/output.txt;										\
		for directory in condition-variable half-duplex-pipe two-stage-mutex work-group; do				\
			echo $$directory: thread pool size $$workers;								\
			sudo perf stat --repeat 10 $$directory/./thread-pool $$workers;						\
			./statistics result/statistics.txt;											\
			rm -f result/statistics.txt;											\
		done;															\
		echo >> result/output.txt;												\
	done;

statistics: statistics.c
	$(CC) $(FLAGS) $< -o $@

plot: statistics throughput-test
	gnuplot result/runtime.gp
	eog result/runtime.png
	rm -f result/output.txt

sync-test: main.c condition-variable/*.[ch] half-duplex-pipe/*.[ch] two-stage-mutex/*.[ch]
	for directory in condition-variable half-duplex-pipe two-stage-mutex work-group; do					\
		echo -n $$directory': '; 												\
		$(CC) $(FLAGS) -DSYNC_TEST=1 -DIMPL="\"$$directory/thread-pool.h\""						\
			main.c $$directory/*.c -o $@ $(LIBRARY);									\
		./$@ 1024;														\
	done;
	rm -f $@

.PHONY: clean

clean:
	rm -f $(EXEC) statistics result/*.txt
