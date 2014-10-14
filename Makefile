CXX = clang++

SDL_LIB = `sdl2-config --libs`
SDL_INCLUDE = `sdl2-config --cflags`
CXXFLAGS = -Wall -Wno-unused-variable -c -std=c++11 $(SDL_INCLUDE)
LDFLAGS = $(SDL_LIB) -lSDL2_image

EXE = sdltut

all: $(EXE)

$(EXE): main.o
	$(CXX) $< $(LDFLAGS) -o $@

main.o: main.cpp sdl.h scopeguard.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm *.o && rm $(EXE)
