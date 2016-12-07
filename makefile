TW: eFFT.o wavio.o thunderWave.o libportaudio.a
	gcc -std=c11 `pkg-config --cflags gtk+-3.0` thunderWave.o eFFT.o wavio.o libportaudio.a -lrt -lm -pthread -lasound -o TW `pkg-config --libs gtk+-3.0`
thunderWave.o: thunderWave.c
	gcc -std=c11 -c `pkg-config --cflags gtk+-3.0` -o thunderWave.o thunderWave.c `pkg-config --libs gtk+-3.0`
wavio.o: wavio.c
	gcc -std=c11 -c -o wavio.o wavio.c
eFFT.o: eFFT.c
	gcc -std=c11 -c -o eFFT.o eFFT.c
clean:
	rm thunderWave.o TW wavio.o
