all:
	gcc `pkg-config --libs --cflags glib-2.0` -lpng -g -otpak -Wall -DDEBUG -O0 ./source/*.c
