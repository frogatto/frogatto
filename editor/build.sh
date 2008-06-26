time g++ -g -I../ -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -L/usr/lib -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lboost_regex `ls -1 ../*.cpp | grep -v main` main.cpp -o editor
