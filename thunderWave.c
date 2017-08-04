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

static int realTimeCallback(const void *input,void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *callData);
static int bufferedCallback(const void *input,void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *callData);


//char * fp="wavs/takeShort.wav";
//char * fp="wavs/sweep.wav";
GObject *window;
GObject *button;
GObject *sButton;
GObject *openButton;
GObject * fileBox;
GObject * fsBox;
GObject * bitBox;
GObject * draw;
GObject * fDraw;
GObject *sScale;
GtkWidget* dialog;

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
	//s->as.p=s->as.start;//realtime
	//s->as.p=0;//buffered;
	printf("AudioLength=%d\n",s->as.datasize);
	printf("Opening Stream\n");
	Pa_OpenDefaultStream(&s->stream,0,1,paInt16,s->as.fs,256,bufferedCallback,&s->as);
	Pa_StartStream(s->stream);
	//while(Pa_IsStreamActive(s->stream));
}

static void stopAudio(GtkWidget *widget, gpointer data){
	guiStuff * s = (guiStuff*)data;
	
	//gtk_widget_queue_draw(s->draw);
	s->as.p=0;
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
	int gl=512*4; //FFT/graph length
	double complex *X;
	double * Y=calloc(gl/2,sizeof(double));
	double * f=calloc(gl/2,sizeof(double));
	complex double *y=calloc(gl,sizeof(complex double));
	for(int i=0;i<gl;i++){
		y[i]=((complex double)(s->as.audio16[s->as.p+i])/32768.0)+(I*0);
	}
	X=fft(y,gl);
	for(int i=0;i<gl/2;i++){
		f[i]=(((double)i)/((double)gl/2.0))*((double)s->as.fs);
		Y[i]=cabs(X[i]);
		Y[i]=Y[i]/4.0;
		//Y[i]=(double)20*log10(cabs(X[i]));
	}
	graphStuff *b=(graphStuff*)data;
	guint width,height;
	GdkRGBA color;
	GtkStyleContext *context;

	context=gtk_widget_get_style_context(widget);
	width=gtk_widget_get_allocated_width(widget);
	height=gtk_widget_get_allocated_height(widget);

	gtk_render_background(context,cr,0,0,width,height);
	cairo_move_to(cr,0,height/2);  
	int diff =(gl/2)/width;
	for(int i=0;i<width;i++){
		cairo_line_to(cr,i,(height-((Y[i+diff]))-(height/2)));
	}
  	gtk_style_context_get_color (context,
                               gtk_style_context_get_state (context),
                               &color);
  	gdk_cairo_set_source_rgba (cr, &color);
	cairo_stroke(cr);
	free(y);
	free(Y);
	free(f);
	return FALSE;


}
static gboolean drawCallback(GtkWidget *widget,cairo_t *cr,gpointer data){
	guiStuff * s = (guiStuff*)data;
	graphStuff *b=(graphStuff*)data;
	guint width,height;
	GdkRGBA color;
	GtkStyleContext *context;
	double x,y;
	context=gtk_widget_get_style_context(widget);
	width=gtk_widget_get_allocated_width(widget);
	height=gtk_widget_get_allocated_height(widget);
	gtk_render_background(context,cr,0,0,width,height);
	cairo_move_to(cr,0,height/2);  
	int diff =(2048)/width;
	for(int i=0,j=0;i<width;i++){
		cairo_line_to(cr,i,(((double)s->as.audio16[j+s->as.p]/32768)*(height-2)/2)+(height/2));
		j+=diff;
	}

  	gtk_style_context_get_color (context,
                               gtk_style_context_get_state (context),
                               &color);
  	gdk_cairo_set_source_rgba (cr, &color);
	cairo_stroke(cr);
	return FALSE;
}
static int bufferedCallback(const void *input,void *output,unsigned long frameCount,const PaStreamCallbackTimeInfo* timeInfo,PaStreamCallbackFlags statusFlags,void *callData){
	audioSignal *b=(audioSignal*)callData;
	short *out =(short*)output;
	gtk_widget_queue_draw((GtkWidget*)draw);
	gtk_widget_queue_draw((GtkWidget*)fDraw);
	double sliderPos=((double)b->p/(double)b->datasize);
	gtk_adjustment_set_value((GtkAdjustment*)sScale,(floorf((sliderPos)*((double)b->audioLength/(double)b->fs*2)*10)/10));
	for(int i=b->p;i<frameCount+b->p;i++){
		*out++=b->audio16[i];
	}
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
	audioSignal *b=(audioSignal*)callData;
	short *out = (short*)output;
	for(int i=b->p; i<frameCount+b->p; i++ ){
		*out++ = readAudio16(b->f);       
	}
	b->p+=frameCount;
	if(b->p>=b->datasize){
		fclose(b->f);
		printf("Finished\n");
		return paComplete;
	}
	else{
		return paContinue;
	}
}
static void sliderChange (GtkAdjustment *adjustment,gpointer data){
	guiStuff *s = (guiStuff*)data;
	gdouble test = gtk_adjustment_get_value(adjustment);
	double sliderPos=floorf((((double)s->as.p/(double)s->as.datasize))*((double)s->as.audioLength/(double)s->as.fs*2)*10)/10;

	if(sliderPos!=test){
		s->as.p=(int)(test*(double)s->as.fs);
	}
	if(Pa_IsStreamActive(s->stream)!=1){
		gtk_widget_queue_draw((GtkWidget*)draw);
		gtk_widget_queue_draw((GtkWidget*)fDraw);
	}
	printf("%f (%f\n)",(double)test,(double)sliderPos);
}

static void openFile(GtkWidget *widget, gpointer data){
	guiStuff * s = (guiStuff*)data;
	char *fp;
	dialog = gtk_file_chooser_dialog_new ("Open File",
                                      (GtkWindow *)window,
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      ("_Cancel"),
                                      GTK_RESPONSE_CANCEL,
                                      ("_Open"),
                                      GTK_RESPONSE_ACCEPT,
                                      NULL);
	if(gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_ACCEPT){
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		fp=gtk_file_chooser_get_filename (chooser);
		printf("%d\n",s->as.datasize);
		if(s->as.datasize!=0){
			free(s->as.audio16);
			s->as.datasize=0;
		}
		s->as;
		readMeta(&s->as,fp);
		readBuffer16(&s->as,fp,s->as.start,s->as.datasize);
		s->as.p=0;
	}
	char * fsStr=calloc(8,sizeof(char));
	char * bitStr=calloc(7,sizeof(char));
	sprintf(fsStr,"%dHz",s->as.fs);
	sprintf(bitStr,"%dBit",(int)s->as.bps);
	gtk_label_set_text((GtkLabel*)fileBox,fp);
	gtk_label_set_text((GtkLabel*)fsBox,fsStr);
	gtk_label_set_text((GtkLabel*)bitBox,bitStr);
	
	free(fsStr);
	free(bitStr);
	gtk_adjustment_set_value((GtkAdjustment*)sScale,0.0);
	gtk_adjustment_set_upper((GtkAdjustment*)sScale,(double)s->as.audioLength/(double)s->as.fs);
	gtk_widget_destroy ((GtkWidget*)dialog);
}

int main (int argc, char *argv[]){
	GtkBuilder *builder;
	
	
	//GObject *label;
	GObject *wGrid;

	char fsStr[8];
	char bitStr[7];
	
	char * fp="wavs/sweep.wav";
	guiStuff g;
	g.as;
	g.as.datasize=0;
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
		/*
	printf("File Info\n");
	printf("audioLength=%d\n",g.as.audioLength);
	printf("datasize=%d\n",g.as.datasize);
	printf("Channels=%d\n",g.as.channels);
	printf("FS=%d\n",g.as.fs);
	printf("AudioStart=%d\n",g.as.start);
	*/
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

	openButton=gtk_builder_get_object(builder,"openButton");
	g_signal_connect(openButton,"clicked",G_CALLBACK(openFile),&g);

	sScale=gtk_builder_get_object(builder,"adjustment1");
	g_signal_connect(sScale,"value-changed",G_CALLBACK(sliderChange),&g);

	


	fileBox=gtk_builder_get_object(builder,"fileLabel");
	fsBox=gtk_builder_get_object(builder,"fsLabel");
	bitBox=gtk_builder_get_object(builder,"bitLabel");

	
	sprintf(fsStr,"%dHz",g.as.fs);
	sprintf(bitStr,"%dBit",(int)g.as.bps);

	gtk_label_set_text((GtkLabel*)fileBox,fp);
	gtk_label_set_text((GtkLabel*)fsBox,fsStr);
	gtk_label_set_text((GtkLabel*)bitBox,bitStr);
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
