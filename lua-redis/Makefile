all:
	gcc -O2 -fPIC -I/usr/include/lua5.4 -c redis.c -o redis.o
	gcc -shared -o redis.so redis.o -lhiredis
