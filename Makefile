# LIBS Generated from:
# 	pkg-config --libs --cflags glfw3 -static
#
# Since we are using `-Wl,--as-needed`, not all of these will actually
# be linked with. However, they might be needed for some features of glfw
# so they shall remain here for now.
LIBS=./glfw/build/src/libglfw3.a -lrt -lm -ldl -lX11 -lpthread -lxcb -lXau -lXdmcp


INCLUDE=-I./include/ -I./rpmalloc/rpmalloc/

# CC=clang
CC=clang

CFLAGS=-O3 -flto -march=native
WARNNINGS=-w
RP_FLAGS=

# CFLAGS=-O0 -ggdb
# WARNNINGS=-Wall -Wextra
# RP_FLAGS=-DENABLE_STATISTICS=1 -DENABLE_ASSERTS=1 -DENABLE_VALIDATE_ARGS=1

main: src/main.c objects/glad.o glfw/build/src/libglfw3.a objects/rpmalloc.o
	$(CC) $(CFLAGS) $(WARNNINGS) -o main src/main.c $(INCLUDE) -Wl,--as-needed $(LIBS) objects/glad.o objects/rpmalloc.o

objects/glad.o: src/glad.c
	$(CC) $(CFLAGS) -w -c src/glad.c -o objects/glad.o $(INCLUDE)

objects/rpmalloc.o: rpmalloc/rpmalloc/rpmalloc.c
	$(CC) $(CFLAGS) $(RP_FLAGS) -w -c rpmalloc/rpmalloc/rpmalloc.c -o objects/rpmalloc.o $(INCLUDE)

glfw/build/src/libglfw3.a:
	mkdir -p ./glfw/build/
	cd glfw/build/; cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS="$(CFLAGS) -w" -DCMAKE_C_COMPILER="$(CC)" ..
	cd glfw/build/; make -j$(nproc)

clean:
	rm -rf glfw/build/
	rm objects/*.o
	rm ./main

.PHONY: clean