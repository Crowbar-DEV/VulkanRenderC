SOURCE=../src/main.c
CFLAGS=-Wall
LFLAGS=-lvulkan -lxcb
INCLUDES=-I/usr/include/vulkan -I/usr/include/xcb

test: $(SOURCE)
	gcc $(SOURCE) $(CFLAGS) $(INCLUDES) $(LFLAGS) -o main