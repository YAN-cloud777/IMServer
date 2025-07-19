all : get put

get:get.cc
	g++ -o $@ $^ -std=c++17 -letcd-cpp-api -lcpprest

put:put.cc
	g++ -o $@ $^ -std=c++17 -letcd-cpp-api -lcpprest

.PHONY:clean
clean:
	rm -f get put