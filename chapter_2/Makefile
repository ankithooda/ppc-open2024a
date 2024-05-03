00_intro:
	mkdir -p bin && g++ -g -o bin/00_intro 00_intro.cpp

00_intro_bench:
	mkdir -p bin && g++ -fopenmp -g -O3 -march=native -std=c++17 -o bin/00_intro_bench 00_intro.cpp

00_intro_asm:
	mkdir -p bin && g++ -S -g -O3 -march=native -std=c++17 00_intro.cpp

00_intro_fn_bench:
	mkdir -p bin && g++ -fopenmp -g -O3 -march=native -std=c++17 -o bin/00_intro_fn_bench 00_intro_fn.cpp

00_intro_fn_asm:
	mkdir -p bin && g++ -S -g -O3 -march=native -std=c++17 00_intro_fn.cpp

00_intro_baseline:
	mkdir -p bin && g++ -fopenmp -g -O3 -march=native -std=c++17 -o bin/00_intro_baseline 00_intro_baseline.cpp

