/*
wavio.c
T.Lloyd 2016

wav file input output
*/
#ifndef WAVIO_H
#define WAVIO_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct{
	int audioLength;
	int bits;
	int bps;
	int blockAlign;
	int byteRate;
	int channels;
	int datasize;
	unsigned int fs;
	int start;
	int p;
	union{
		short int * audio16;
		char * audio8;
	};
	FILE *f;
}audioSignal;

//Misc Functions
int hex2dec4Byte(unsigned char byte3, unsigned char byte2, unsigned char byte1, unsigned char byte0);
int hex2dec3Byte(unsigned char byte2, unsigned char byte1, unsigned char byte0);
int hex2dec2Byte(unsigned char byte1, unsigned char byte0);

//File meta i/o
void readMeta(audioSignal * p,char * fp);
void writeMeta(audioSignal * p,char * fp);

//Read individual audio samples
short int readAudio16(FILE * f);
char readAudio8(FILE * f);

//Read AudioBuffers;
void readBuffer16(audioSignal *p,char *fp,int start,int size);

//Write audio buffer
void writeAudio(audioSignal * p,char * fp);

#endif // WAVIO_H
