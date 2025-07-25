all:publish consume

publish:publish.cc
	g++ -o $@ $^ -std=c++17 -lev -lamqpcpp

consume:comsume.cc
	g++ -o $@ $^ -std=c++17 -lev -lamqpcpp
	
.PHONY:clean
clean:
	rm -f publish consume