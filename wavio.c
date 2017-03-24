/*
*		wavio.c
*		T.Lloyd 2016
*
*		wav file input output
*/
#include "wavio.h"


void readMeta(audioSignal * p,char * fp){
	FILE *f;
	f=fopen(fp,"rb");
	if(f==NULL){
		printf("File not found");
		return;
	}
	char tag[5];//Array to dump unwanted data;
	//RIFF HEADER
	fread(tag,sizeof(char),4,f);
	//CHUNKSIZE
	int chunksize=hex2dec4Byte(fgetc(f),fgetc(f),fgetc(f),fgetc(f));
	//WAVEHEADER
	fread(tag,sizeof(char),4,f);
	//fmt
	fread(tag,sizeof(char),4,f);
	//chunk1size
	int chunk1size=hex2dec4Byte(fgetc(f),fgetc(f),fgetc(f),fgetc(f));
	int audioformat=hex2dec2Byte(fgetc(f),fgetc(f));
	p->channels=hex2dec2Byte(fgetc(f),fgetc(f));
	p->fs=hex2dec4Byte(fgetc(f),fgetc(f),fgetc(f),fgetc(f));
	p->byteRate=hex2dec4Byte(fgetc(f),fgetc(f),fgetc(f),fgetc(f));
	p->blockAlign=hex2dec2Byte(fgetc(f),fgetc(f));
	p->bps=hex2dec2Byte(fgetc(f),fgetc(f));
	//Tag skipping code here
	fread(tag,sizeof(char),4,f);
	//tag[4]='\0';
	printf("fs = %d\n",p->fs);
	printf("%c",tag[0]);
	printf("%c",tag[1]);
	printf("%c",tag[2]);
	printf("%c\n",tag[3]);
	printf("strcmpresult=%d\n",strcmp(tag,"data"));
	printf("Searching for data tag...\n");
	int tempSize;
	while(strcmp(tag,"data")!=0){
		tempSize=hex2dec4Byte(fgetc(f),fgetc(f),fgetc(f),fgetc(f));
		for(int i=0;i<tempSize;i++){
			fgetc(f);
		}
		fread(tag,sizeof(char),4,f);
	}
	printf("Loop Finished\n");
	p->datasize=hex2dec4Byte(fgetc(f),fgetc(f),fgetc(f),fgetc(f));
	p->audioLength=p->datasize/p->blockAlign;
	p->start=ftell(f);
	fclose(f);
}
short int readAudio16(FILE * f){
	//return (fgetc(f)<<8) + fgetc(f);
	return fgetc(f)+(fgetc(f)*16*16);
}
char readAudio8(FILE * f){
	return fgetc(f);
}
void writeMeta(audioSignal * p,char * fp){
	//calculate chunk sizes;
	int chunk2size=p->audioLength*p->channels*(p->bps/8);
	int chunk1size=16; //Only working with PCM audio
	int chunksize=36+(8+chunk1size)+(8+chunk2size);
	int audioFormat =1;

	FILE * f;
	f=fopen(fp,"wb");
	//------------------Riff Header------------------
	fputs("RIFF",f);
	//chunksize
	for(int i=0;i<4;i++){
		fputc((char)(chunksize>>(8*i)),f);
	}
	printf("\n");
	//WAVE format
	fputs("WAVE",f);
	//------------------First Subchunk------------------
	//subchunk1ID
	fputs("fmt ",f);
	//subchunk1size
	for(int i=0;i<4;i++){
		fputc((char)(chunk1size>>(8*i)),f);
	}
	//audioFormat
	for(int i=0;i<2;i++){
		fputc((char)(audioFormat>>(8*i)),f);
	}
	//Number of Channels
	for(int i=0;i<2;i++){
		fputc((char)(p->channels>>(8*i)),f);
	}
	//SampleRate
	for(int i=0;i<4;i++){
		fputc((char)(p->fs>>(8*i)),f);
	}
	//ByteRate
	for(int i=0;i<4;i++){
		fputc((char)(p->byteRate>>(8*i)),f);
	}
	//BlockAlign
	for(int i=0;i<2;i++){
		fputc((char)(p->blockAlign>>(8*i)),f);
	}
	//BPS
	for(int i=0;i<2;i++){
		fputc((char)(p->bps>>(8*i)),f);
	}
	//------------------Second Subchunk------------------
	//subchunk1ID
	fputs("data",f);
	//subchunk2Size
	for(int i=0;i<4;i++){
		fputc((char)(chunk2size>>(8*i)),f);
	}
	//data
	for(int j=0;j<p->audioLength*p->channels;j++){
		for(int i=0;i<p->bps/8;i++){
			fputc((char)(p->audio16[j]>>(8*i)),f);
			//printf("%d,",(char)(p->audio16[j]>>(8*i)));
		}
	}
	fclose(f);
}
void readBuffer16(audioSignal * p,char * fp,int start,int size){
	p->audio16=calloc(size,sizeof(short int));
	FILE *f=fopen(fp,"rb");
	for(int i=0;i<start;i++){
		fgetc(f);
	}
	for(int i=0;i<size;i++){
		p->audio16[i]=readAudio16(f);
	}
	fclose(f);
}
void writeAudio(audioSignal * p,char * fp){
	printf("huff");
}
int hex2dec4Byte(unsigned char byte3, unsigned char byte2, unsigned char byte1, unsigned char byte0){
	return (byte3<<24) + (byte2<<16) + (byte1<<8) + byte0;
}
int hex2dec3Byte(unsigned char byte2, unsigned char byte1, unsigned char byte0){
	return (byte2<<16) + (byte1<<8) + byte0;
}
int hex2dec2Byte(unsigned char byte1, unsigned char byte0){
	return (byte1<<8) + byte0;
}
