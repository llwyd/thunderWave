/*

	thunderWave
	
	Basic spectrum analyser for wav files

	T.Lloyd 2016
	

*/
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <complex.h>
#include "portaudio.h"
#include "wavio.h"
#include "eFFT.h"

#define M_PI 3.141592653589793

typedef struct{
	short int *sin;
	int fs;
	int p;
	int l;
}audioData;
typedef struct{
	GObject *text;
	PaStream *stream;
	audioData a;
	audioSignal as;
	FILE *f;//Audio fileStream
}guiStuff;
typedef struct{
	FILE *f;
	char * path;
	int start;
}graphStuff;
//GObject *label;

static int realTimeCallback(const void *input,void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *callData);
static int bufferedCallback(const void *input,void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *callData);
short int * gensin(double fs,int l,double f);

char * fp="wavs/sweep.wav";

GObject * draw;
GObject * fDraw;

static void playAudio(GtkWidget *widget, gpointer data){
	guiStuff * s = (guiStuff*)data;	
	g_print ("Play Audio!\n");
	//gchar * m = ("MeowMEowMEow");
	//gtk_label_set_text(GTK_LABEL(s->text),m);
	
        //a.sin=gensin(48000.0,48000*2,2000);
	//a.fs=48000;
        //a.p=0; 

	Pa_Initialize();
	/*
	s->as.f=fopen(fp,"r");
	for(int i=0;i<s->as.start;i++){
		fgetc(s->as.f);
	}
	*/
	s->as.p=s->as.start;//realtime
	s->as.p=0;//buffered;
//	readBuffer16(&s->as,fp,s->as.start,s->as.datasize);
	printf("AudioLength=%d\n",s->as.datasize);
	printf("Opening Stream\n");
	//Pa_OpenDefaultStream(&s->stream,0,1,paInt16,s->as.fs,256,realTimeCallback,&s->as);
	Pa_OpenDefaultStream(&s->stream,0,1,paInt16,s->as.fs,256,bufferedCallback,&s->as);
	Pa_StartStream(s->stream);
//	while(Pa_IsStreamActive(s->stream));
	//Pa_StopStream(s->stream);
	//Pa_CloseStream(s->stream);
	//Pa_Terminate();
	
}

static void stopAudio(GtkWidget *widget, gpointer data){
	guiStuff * s = (guiStuff*)data;
	
	//gtk_widget_queue_draw(s->draw);
	
	printf("STOP!");
	Pa_StopStream(s->stream);
	Pa_CloseStream(s->stream);
	Pa_Terminate();
	
}

static void quitProgram(GtkWidget *widget, gpointer data){
	guiStuff *s = (guiStuff*)data;
	Pa_StopStream(s->stream);
	Pa_CloseStream(s->stream);
	Pa_Terminate();
	gtk_main_quit();
}

static gboolean fftCallback(GtkWidget *widget,cairo_t *cr,gpointer data){
	guiStuff * s = (guiStuff*)data;
	printf("FFT\n");
	int gl=2048; //FFT/graph length
	double complex *X;
	int * Y=calloc(gl/2,sizeof(double complex));
	int * f=calloc(gl/2,sizeof(double complex));
	//short complex x[512];
	double complex *y=calloc(gl,sizeof(double complex));
	//	memcpy(x,s->as.audio16+s->as.p,512*sizeof(short));
	for(int i=0;i<gl;i++){
		y[i]=(complex double)s->as.audio16[s->as.p]/32767;
	}
	X=fft(y,gl);
	printf("%f.5+i%f.5",creal(X[0]),cimag(X[0]));
	for(int i=0;i<gl/2;i++){
		f[i]=((double)i / (gl/ 2))*(s->as.fs / 2);
		Y[i]=abs(X[i]);
	}
}

static gboolean drawCallback(GtkWidget *widget,cairo_t *cr,gpointer data){
	guiStuff * s = (guiStuff*)data;
	//audioData * b = (audioData*)data;
//	 /*
	graphStuff *b=(graphStuff*)data;
	guint width,height;
	GdkRGBA color;
	GtkStyleContext *context;
	double x,y;
	//printf("Drawing Callback Activated\n");
	context=gtk_widget_get_style_context(widget);
	width=gtk_widget_get_allocated_width(widget);
	height=gtk_widget_get_allocated_height(widget);

	gtk_render_background(context,cr,0,0,width,height);

	//cairo_new_path(cr);
	cairo_move_to(cr,0,height/2);  
//	cairo_line_to(cr,width/4,height/4);
//	cairo_curve_to(cr,width/2,height/3,width/3,height/4,width,height);
	//int diff=b->l/width;
	int diff =128/width;

	//printf("%d\n",diff);
	for(int i=0;i<width;i++){
		//cairo_line_to(cr,i,(((double)b->sin[i*diff]/32767)*(height-2)/2)+(height/2));
		//cairo_line_to(cr,i,(((double)readAudio16(b->f)/32767)*(height-2)/2)+(height/2));
		cairo_line_to(cr,i,(((double)s->as.audio16[i+s->as.p]/32767)*(height-2)/2)+(height/2));
	}

  	gtk_style_context_get_color (context,
                               gtk_style_context_get_state (context),
                               &color);
  	gdk_cairo_set_source_rgba (cr, &color);
	cairo_stroke(cr);
 // cairo_fill (cr);
 // */
	return FALSE;
}
static int bufferedCallback(const void *input,void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *callData){
	audioSignal *b=(audioSignal*)callData;
	short *out =(short*)output;
	(void) timeInfo;
	(void) statusFlags;
	(void) input;
	//printf("%d,",b->p);
	for(int i=b->p;i<frameCount+b->p;i++){
		*out++=b->audio16[i];
		//printf("%d\n",b->audio16[i]);
	}
	gtk_widget_queue_draw((GtkWidget*)draw);
	b->p+=frameCount;
	if(b->p>=b->datasize){
		printf("Finished!");
		return paComplete;
	}
	else{
		return paContinue;
	}
}

static int realTimeCallback(const void *input,void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *callData){
	//audioData *b = (audioData*)callData;
	//guiStuff *b=(guiStuff*)callData;
	audioSignal *b=(audioSignal*)callData;
	short *out = (short*)output;
	(void) timeInfo;
	(void) statusFlags;
	(void) input;
//	printf("\n\n----fs=%d----\n\n",b->fs);
//	short int posi=b->p;
	for(int i=b->p; i<frameCount+b->p; i++ ){
        	//*out++ = b->sin[i];
		*out++ = readAudio16(b->f);
//		*out++ = fgetc(b->f)+(fgetc(b->f)*16*16);
//		printf("%d\n",readAudio16(b->f));
//		printf("%d\n",i);          
	}
	b->p+=frameCount;
//	printf("%d,",b->p);
	if(b->p>=b->datasize){
		fclose(b->f);
		printf("Finished\n");
		return paComplete;
	}
	else{
//		printf("%d,",b->p);
		return paContinue;
	}
	//return paContinue;
}

short int * gensin(double fs,int l,double f){
    //generates a sine wave requiring sampling frequency, length and frequency.
        //float* s = malloc(sizeof(float)*l);
  	short int *x=malloc(sizeof(short int)*l);
  	float s;
        float T=1/fs;
        for(int i=0;i<l;i++){
                s= (float)sin((double)2*M_PI*i*T*f);
    		x[i]=(short)((double)s*(double)32767);
        }
  	printf("Sine Wave Created\n");
	return x;
 }


int main (int argc, char *argv[]){
	GtkBuilder *builder;
	GObject *window;
	GObject *button;
	GObject *sButton;
	//GObject *label;
	GObject *wGrid;
	
	guiStuff g,k;
	
	k.a.l=48878*2;
	k.a.sin=gensin(48000.0,g.a.l,500);
	k.a.p=0;
	k.a.fs=48000;

	g.as;
	readMeta(&g.as,fp);
	readBuffer16(&g.as,fp,g.as.start,g.as.datasize);
	g.as.p=0;
	//g.as.g=fopen(fp,"r");
	

/*
	graphStuff t;
	t.path="sweep.wav";
	t.start=g.as.start;
	t.f=fopen(t.path,"r");
	for(int i=0;i<t.start;i++){
		fgetc(t.f);
	}*/

	printf("File Info\n");
	printf("audioLength=%d\n",g.as.audioLength);
	printf("datasize=%d\n",g.as.datasize);
	printf("Channels=%d\n",g.as.channels);
	printf("FS=%d\n",g.as.fs);
	printf("AudioStart=%d\n",g.as.start);
	gtk_init (&argc, &argv);

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, "thunderWaveBasic.ui", NULL);

	window = gtk_builder_get_object (builder, "window1");
	//g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect (window, "destroy", G_CALLBACK (quitProgram), &g);
	
//	wGrid=gtk_builder_get_object(builder,"grid1");
	//g.text = gtk_builder_get_object(builder,"pos");


	//GObject *draw1=gtk_builder_get_object(builder,"drawingarea1");
	draw=gtk_builder_get_object(builder,"drawingarea1");
	g_signal_connect(draw,"draw",G_CALLBACK(drawCallback),&g);
	fDraw=gtk_builder_get_object(builder,"drawingarea2");
	g_signal_connect(draw,"draw",G_CALLBACK(fftCallback),&g);

	button = gtk_builder_get_object(builder,"playButton");
	g_signal_connect(button,"clicked",G_CALLBACK(playAudio),&g);	

	sButton=gtk_builder_get_object(builder,"stopButton");
	g_signal_connect(sButton,"clicked",G_CALLBACK(stopAudio),&g);
	//g.text = gtk_builder_get_object(builder,"pos");


	//Pa_Initialize();
	//Pa_Terminate();
	g_object_unref(builder);
	gtk_widget_show((GtkWidget*) window);
	gtk_main();

	return 0;
}
