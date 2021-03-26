debug:
	g++ -g -Wall -Wextra -Wpedantic -DDEBUG -o main main.cpp bitboards.cpp position.cpp movegen.cpp evaluation.cpp search.cpp uci.cpp tt.cpp zobrist.cpp material.cpp movepick.cpp
release:
	g++ -g -O3 -Wall -Wextra -pedantic -o main main.cpp bitboards.cpp position.cpp movegen.cpp evaluation.cpp search.cpp uci.cpp tt.cpp zobrist.cpp material.cpp movepick.cpp
clean:
	rm -f *.o main.exe