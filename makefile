exe:	main.cpp
	g++ *.cpp src/*.cpp -std=c++14 -Isrc -O3

debug:	main.cpp
	g++ *.cpp src/*.cpp -std=c++14 -Isrc -O3 -fsanitize=undefined

run:
	./a.out

r:
	./a.out

clean:
	rm a.out

c:
	rm a.out