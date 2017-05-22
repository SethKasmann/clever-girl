exe:	main.cpp
	g++ *.cpp src/*.cpp -std=c++14 -Isrc -O2

run:
	./a.out

r:
	./a.out

clean:
	rm a.out

c:
	rm a.out