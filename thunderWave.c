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

char * fp="wavs/session.wav";
//char * fp="wavs/sweep.wav";

GObject * draw;
GObject * fDraw;
GObject *sScale;

static void playAudio(GtkWidget *widget, gpointer data){
	guiStuff * s = (guiStuff*)data;	
	g_print ("Play Audio!\n");

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
	//printf("FFT\n");
	int gl=512*8; //FFT/graph length
	double complex *X;
	double * Y=calloc(gl/2,sizeof(double));
	double * f=calloc(gl/2,sizeof(double));
	//short complex x[512];
	complex double *y=calloc(gl,sizeof(complex double));
	//	memcpy(x,s->as.audio16+s->as.p,512*sizeof(short));
	for(int i=0;i<gl;i++){
		y[i]=((complex double)(s->as.audio16[s->as.p+i])/32767.0)+(I*0);
	}
	X=fft(y,gl);
	//printf("%f+i%f",creal(X[0]),cimag(X[0]));
	for(int i=0;i<gl/2;i++){
		f[i]=(((double)i)/((double)gl/2.0))*((double)s->as.fs);
		Y[i]=cabs(X[i]);
		Y[i]=Y[i]/4.0;
		//Y[i]=(double)20*log10(cabs(X[i]));
		if(Y[i]!=0){
//			printf("(%f,%fHz)",Y[i],f[i]);
		}
	}
//	printf("\n");
	//GRAPH PLOTTING
	//printf("%d,%d\n,",f[0],Y[0]);
	graphStuff *b=(graphStuff*)data;
	guint width,height;
	GdkRGBA color;
	GtkStyleContext *context;

	context=gtk_widget_get_style_context(widget);
	width=gtk_widget_get_allocated_width(widget);
	height=gtk_widget_get_allocated_height(widget);

	gtk_render_background(context,cr,0,0,width,height);

	//cairo_new_path(cr);
	cairo_move_to(cr,0,height/2);  
	int diff =(gl/2)/width;

	//printf("%d\n",diff);
	for(int i=0;i<width;i++){
		//cairo_line_to(cr,i,((Y[(i+diff)*width]/(gl/4))*-1*(height-2)/2)+(height/2));
		cairo_line_to(cr,i,(height-((Y[i+diff]))-(height/2)));
	}

  	gtk_style_context_get_color (context,
                               gtk_style_context_get_state (context),
                               &color);
  	gdk_cairo_set_source_rgba (cr, &color);
	cairo_stroke(cr);
 // cairo_fill (cr);
 // */

	free(y);
	free(Y);
	free(f);
	
	return FALSE;


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
	//int diff=b->l/width;
	int diff =(2048)/width;

	//printf("%d\n",diff);
	for(int i=0,j=0;i<width;i++){
		//cairo_line_to(cr,i,(((double)b->sin[i*diff]/32767)*(height-2)/2)+(height/2));
		//cairo_line_to(cr,i,(((double)readAudio16(b->f)/32767)*(height-2)/2)+(height/2));
		cairo_line_to(cr,i,(((double)s->as.audio16[j+s->as.p]/32767)*(height-2)/2)+(height/2));
		j+=diff;
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
	//printf("%d,",b->p);
		gtk_widget_queue_draw((GtkWidget*)draw);
	gtk_widget_queue_draw((GtkWidget*)fDraw);
		double sliderPos=((double)b->p/(double)b->datasize);
	gtk_adjustment_set_value((GtkAdjustment*)sScale,(floorf((sliderPos)*((double)b->audioLength/(double)b->fs*2)*10)/10));

	for(int i=b->p;i<frameCount+b->p;i++){
		*out++=b->audio16[i];
		//printf("%d\n",b->audio16[i]);
	}

	//gtk_adjustment_set_value((GtkAdjustment*)sScale,floor(((double)b->p/(double)b->datasize)*((double)b->audioLength/(double)b->fs)));
	//double sliderPos=floorf(((double)b->p/(double)b->datasize)*100)/100;
	b->p+=frameCount;
	if(b->p>=b->audioLength){
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


static void sliderChange (GtkAdjustment *adjustment,gpointer data){
	guiStuff *s = (guiStuff*)data;
	gdouble test = gtk_adjustment_get_value(adjustment);
	double sliderPos=floorf((((double)s->as.p/(double)s->as.datasize))*((double)s->as.audioLength/(double)s->as.fs*2)*10)/10;

	if(sliderPos!=test){
		s->as.p=(int)(test*(double)s->as.fs);
	}
	printf("%f (%f\n)",(double)test,(double)sliderPos);
}

int main (int argc, char *argv[]){
	GtkBuilder *builder;
	GObject *window;
	GObject *button;
	GObject *sButton;

	GObject * fileBox;
	GObject * fsBox;
	GObject * channelBox;
	
	//GObject *label;
	GObject *wGrid;

	char fsStr[8];
	
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
	gtk_builder_add_from_file (builder, "thunderWaveAdvanced.ui", NULL);

	window = gtk_builder_get_object (builder, "window1");
	//g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect (window, "destroy", G_CALLBACK (quitProgram), &g);
	
//	wGrid=gtk_builder_get_object(builder,"grid1");
	//g.text = gtk_builder_get_object(builder,"pos");


	//GObject *draw1=gtk_builder_get_object(builder,"drawingarea1");
	draw=gtk_builder_get_object(builder,"drawingarea1");
	g_signal_connect(draw,"draw",G_CALLBACK(drawCallback),&g);
	
	fDraw=gtk_builder_get_object(builder,"drawingarea2");
	g_signal_connect(fDraw,"draw",G_CALLBACK(fftCallback),&g);

	button = gtk_builder_get_object(builder,"playButton");
	g_signal_connect(button,"clicked",G_CALLBACK(playAudio),&g);	

	sButton=gtk_builder_get_object(builder,"stopButton");
	g_signal_connect(sButton,"clicked",G_CALLBACK(stopAudio),&g);

	sScale=gtk_builder_get_object(builder,"adjustment1");
	g_signal_connect(sScale,"value-changed",G_CALLBACK(sliderChange),&g);

	fileBox=gtk_builder_get_object(builder,"fileLabel");
	fsBox=gtk_builder_get_object(builder,"fsLabel");
	channelBox=gtk_builder_get_object(builder,"channelLabel");

	
	sprintf(fsStr,"%dHz",g.as.fs);

	gtk_label_set_text((GtkLabel*)fileBox,fp);
	gtk_label_set_text((GtkLabel*)fsBox,fsStr);
	gtk_adjustment_set_value((GtkAdjustment*)sScale,0.0);
	gtk_adjustment_set_upper((GtkAdjustment*)sScale,(double)g.as.audioLength/(double)g.as.fs);




	//g.text = gtk_builder_get_object(builder,"pos");


	//Pa_Initialize();
	//Pa_Terminate();
	g_object_unref(builder);
	gtk_widget_show((GtkWidget*) window);
	gtk_main();

	return 0;
}
