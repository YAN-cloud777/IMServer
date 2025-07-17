async:asyn.cc
	g++ -o $@ $^ -std=c++17 -lspdlog -lfmt

sync:sync.cc
	g++ -o $@ $^ -std=c++17 -lspdlog -lfmt

.PHONY:clean
clean:
	rm -f async sync *.log