/*
 * ui.c
 *
 *  Created on: Nov 21, 2021
 *      Author: sydxrey
 *
 *
 *      LCD and touchscreen management for project User Interface.
 */


///********************************************///
/// Commencer le rapport de projet (tutoriel)
///********************************************///


#include <ui.h>
#include <math.h>
#include <audio.h>
#include <stdio.h>
#include <stdlib.h>
#include "bsp/disco_lcd.h"
#include "bsp/disco_ts.h"
#include "bsp/disco_base.h"
#include "arm_math.h"

extern double inputLevelLavr;
extern double inputLevelRavr;
extern float FFTInput[];
extern float FFTOutput[];
extern float FFTOutputMag[];

#define FFTLength 256
int delayMs = 0;
int delayFeed = 0;
int volume = 50;
float depth = 0.5;	//profondeur ?
int time = 0;		//(1/fréquence) ?

/**
 * Display basic UI information.
 */
void uiDisplayBasic(void) {

	LCD_Clear(LCD_COLOR_WHITE);

	LCD_SetStrokeColor(LCD_COLOR_BLACK);
	LCD_SetBackColor(LCD_COLOR_WHITE);
	LCD_SetFont(&Font16);
	LCD_DrawString(0, 0, (uint8_t*) "CHOISISSEZ VOS PARAMETRES:", CENTER_MODE, true);         //MODIFICATION POUR LE TITRE


/// DEBUT PREMIERE LIGNE DE SLIDERS DE L'INTERFACE
	/* flanger range */
	LCD_DrawRect(40,35,50,80);
	/* chorus range */
	LCD_DrawRect(120,35,50,80);
	/* phaser range */
	LCD_DrawRect(200,35,50,80);
	/* reverb range */
	LCD_DrawRect(280,35,50,80);
	/* volume range */
	LCD_DrawRect(360,35,50,80);
/// FIN PREMIERE LIGNE DE SLIDERS DE L'INTERFACE

/// DEBUT DEUXIEME LIGNE DE SLIDERS DE L'INTERFACE
	/* rate (frequence) range */
	LCD_DrawRect(40,150,50,80);
	/* depth (profondeur) range */
	LCD_DrawRect(120,150,50,80);
	/* delay (retard) range */
	LCD_DrawRect(200,150,50,80);
	/* feedback range */
	LCD_DrawRect(280,150,50,80);
	/* "en plus" range */
	LCD_DrawRect(360,150,50,80);
/// FIN DEUXIEME LIGNE DE SLIDERS DE L'INTERFACE


	/* Init Vol, Delay, Feed */
	LCD_SetFont(&Font12); 			//pour la police

/// DEBUT PREMIERE LIGNE DE SLIDERS DE L'INTERFACE
	LCD_DrawString(40,20,(uint8_t *) "Flanger", LEFT_MODE, true);
	LCD_DrawString(120,20,(uint8_t *) "Chorus", LEFT_MODE, true);
	LCD_DrawString(200,20,(uint8_t *) "Phaser", LEFT_MODE, true);
	LCD_DrawString(280,20,(uint8_t *) "Reverb", LEFT_MODE, true);
	LCD_DrawString(360,20,(uint8_t *) "Volume", LEFT_MODE, true);
/// FIN PREMIERE LIGNE DE SLIDERS DE L'INTERFACE

/// DEBUT DEUXIEME LIGNE DE SLIDERS DE L'INTERFACE
	LCD_DrawString(40,135,(uint8_t *) "Rate", LEFT_MODE, true);
	LCD_DrawString(120,135,(uint8_t *) "Depth", LEFT_MODE, true);
	LCD_DrawString(200,135,(uint8_t *) "Delay", LEFT_MODE, true);
	LCD_DrawString(280,135,(uint8_t *) "Feedback", LEFT_MODE, true);
	LCD_DrawString(360,135,(uint8_t *) "?????", LEFT_MODE, true);
/// FIN DEUXIEME LIGNE DE SLIDERS DE L'INTERFACE

	LCD_SetFillColor(LCD_COLOR_RED);

/// DEBUT PREMIERE LIGNE DE SLIDERS DE L'INTERFACE
	LCD_FillRect(41,40+35,49,80-40); //FLANGER
	LCD_FillRect(121,40+35,49,80-40); //CHORUS
	LCD_FillRect(201,40+35,49,80-40); //PHASER
	LCD_FillRect(281,40+35,49,80-40); //REVERB
	LCD_FillRect(361,40+35,49,80-40); //VOLUME
/// FIN PREMIERE LIGNE DE SLIDERS DE L'INTERFACE

/// DEBUT DEUXIEME LIGNE DE SLIDERS DE L'INTERFACE
	LCD_FillRect(41,40+150,49,80-40); //RATE FREQUENCE
	LCD_FillRect(121,40+150,49,80-40); //DEPTH PROFONDEUR
	LCD_FillRect(201,40+150,49,80-40); //DELAY RETARD
	LCD_FillRect(281,40+150,49,80-40); //FEEDBACK
	LCD_FillRect(361,40+150,49,80-40); //???
/// FIN DEUXIEME LIGNE DE SLIDERS DE L'INTERFACE

}

/*
 * Display All Params
 */

                                                                         /// FLANGER
static void printFlanger(uint32_t x){
	char flanger_char[10];
	sprintf(flanger_char, "%lu %%",x);
	uint16_t X = 40;
	uint16_t Y = 20;
	LCD_DrawString(X,Y,(uint8_t *)flanger_char, LEFT_MODE, true);
}

                                                                         /// CHORUS
static void printChorus(uint32_t x){
	char chorus_char[10];
	sprintf(chorus_char, "%lu %%",x);
	uint16_t X = 120;
	uint16_t Y = 20;
	LCD_DrawString(X,Y,(uint8_t *)chorus_char, LEFT_MODE, true);
}

                                                                         /// PHASER
static void printPhaser(uint32_t x){
	char phaser_char[10];
	sprintf(phaser_char, "%lu %%",x);
	uint16_t X = 200;
	uint16_t Y = 20;
	LCD_DrawString(X,Y,(uint8_t *)phaser_char, LEFT_MODE, true);
}

                                                                         /// REVERB
static void printReverb(uint32_t x){
	char reverb_char[10];
	sprintf(reverb_char, "%lu %%",x);
	uint16_t X = 280;
	uint16_t Y = 20;
	LCD_DrawString(X,Y,(uint8_t *)reverb_char, LEFT_MODE, true);
}

                                                                         /// VOLUME
static void printVolume(uint32_t x){
	char volume_char[10];
	sprintf(volume_char, "%lu %%",x);
	uint16_t X = 360;
	uint16_t Y = 20;
	LCD_DrawString(X,Y,(uint8_t *)volume_char, LEFT_MODE, true);
}

                                                                         /// RATE
static void printRate(uint32_t x){
	char rate_char[10];
	sprintf(rate_char, "%lu %%",x);
	uint16_t X = 40;
	uint16_t Y = 135;
	LCD_DrawString(X,Y,(uint8_t *)rate_char, LEFT_MODE, true);
}

                                                                         /// DEPTH
static void printDepth(uint32_t x){
	char depth_char[10];
	sprintf(depth_char, "%lu %%",x);
	uint16_t X = 120;
	uint16_t Y = 135;
	LCD_DrawString(X,Y,(uint8_t *)depth_char, LEFT_MODE, true);
}

                                                                         /// DELAY
static void printDelay(uint32_t x){
	char delay_char[10];
	sprintf(delay_char, "%lu %%",x);
	uint16_t X = 200;
	uint16_t Y = 135;
	LCD_DrawString(X,Y,(uint8_t *)delay_char, LEFT_MODE, true);
}

                                                                         /// FEEDBACK
static void printFeedback(uint32_t x){
	char feedback_char[10];
	sprintf(feedback_char, "%lu %%",x);
	uint16_t X = 280;
	uint16_t Y = 135;
	LCD_DrawString(X,Y,(uint8_t *)feedback_char, LEFT_MODE, true);
}


///**********************************************************************///
///                          VALEUR DE FLANGER                           ///
///**********************************************************************///

void uiSliderFlanger(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 40;
				int y = 35;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE CHORUS                            ///
///**********************************************************************///

void uiSliderChorus(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 120;
				int y = 35;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE PHASER                            ///
///**********************************************************************///

void uiSliderPhaser(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 200;
				int y = 35;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE REVERB                            ///
///**********************************************************************///

void uiSliderReverb(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 280;
				int y = 35;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE RANGE                             ///
///**********************************************************************///

void uiSliderRange(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 360;
				int y = 35;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE RATE                              ///
///**********************************************************************///

void uiSliderRate(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 40;
				int y = 150;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE DEPTH                             ///
///**********************************************************************///

void uiSliderDepth(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 120;
				int y = 150;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE DELAY                             ///
///**********************************************************************///

void uiSliderDelay(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 200;
				int y = 150;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}

///**********************************************************************///
///                          VALEUR DE FEEDBACK                          ///
///**********************************************************************///

void uiSliderFeedback(){
	TS_StateTypeDef  TS_State;

	TS_GetState(&TS_State);

	/* detect touch event */
	if(TS_State.touchDetected){

				/* Get X and Y position of the touch post calibrated */
				uint16_t xt = TS_State.touchX[0];
				uint16_t yt = TS_State.touchY[0];
				/* Init position and size of elements */
				int x = 280;
				int y = 150;
				int w = 50;
				int h = 80;

				/* If touch is in the delay bar */
				if(xt>x && xt<x+w && yt>y && yt<y+h){
					/* Position x into delay value in ms */
					delayMs=((xt-x)*10)/2;
					/* Display new delay value */
					printDelay((uint32_t) delayMs);

					/* RED Bar for level */
 					LCD_SetFillColor(LCD_COLOR_RED);
					LCD_FillRect(x+1,y+1,xt-x,h-1);
					/* WHITE Bar for update */
					LCD_SetFillColor(LCD_COLOR_WHITE);
					LCD_FillRect(xt,y+1,w+x-xt,h-1);
				}
	}
}



                                                                              ///EN COMMENTAIRE
/**
 * Displays line or microphones input level on the LCD.
 */
void uiDisplayInputLevel() {

	uint8_t buf[50];

	LCD_SetStrokeColor(LCD_COLOR_BLACK);
	LCD_SetBackColor(LCD_COLOR_WHITE);
	LCD_SetFont(&Font12);

	if (inputLevelLavr > 0) {
		int lvl_db = (int) (20. * log10(inputLevelLavr));
		sprintf((char*) buf, "%d ", lvl_db);
		LCD_DrawString(5, 15, (uint8_t*) buf, LEFT_MODE, true);
	} else
		LCD_DrawString(5, 15, (uint8_t*) "-inf", LEFT_MODE, true);

	if (inputLevelRavr > 0) {
		int lvl_db = (int) (20. * log10(inputLevelRavr));
		sprintf((char*) buf, "%d ", lvl_db);
		LCD_DrawString(30, 15, (uint8_t*) buf, LEFT_MODE, true);
	} else
		LCD_DrawString(30, 15, (uint8_t*) "-inf", LEFT_MODE, true);
}


                                                                              ///EN COMMENTAIRE
/**
 * Displays spectrogram.
 */

uint32_t VAL;
void uiDisplaySpectrum() {
	/* sweep reset */
	if(time>360){
		time=0;
	}
	/* sweep init position */
	uint16_t x=time+60;
	uint16_t y=270;
	uint32_t color;

	/* run through mag values */
	for(int i=0;i<(256/2);i++){
		/* Amplitude of frequency "i" */
		VAL = (uint32_t) (FFTOutputMag[i]*255);
		color=(uint32_t) 4278190080+(VAL);

		/* shade of blue */
		LCD_DrawPixelColor(x,y-(i),(uint32_t) color);
		/* tracking bar */
		LCD_DrawPixelColor(x+1,y-(i),(uint32_t) (4278190080+16753920));
	}
	/* new sweep position */
	time=time+1;
}


                                                                              ///EN COMMENTAIRE
/**
 * Displays Master VU level.
 */
void uiDisplayMaster() {
	int dbL = (int) (20. * log10(inputLevelLavr));
	int dbR = (int) (20. * log10(inputLevelRavr));

	uiDisplayInputLevel();

	int L = -2*(dbL+60)-100; // augmentation de dynamique pour l'affichage
	int R =	-2*(dbR+60)-100; //

	/* Refresh bar */
	LCD_SetFillColor(LCD_COLOR_WHITE);
	LCD_FillRect(5, 30, 20, (270+L));
	LCD_FillRect(35, 30, 20 , (270+R));
	/* Value bar */
	LCD_SetFillColor(LCD_COLOR_RED);
	LCD_FillRect(5, 270+L, 20, LCD_SCREEN_HEIGHT-(270+L));
	LCD_FillRect(35, 270+R, 20, LCD_SCREEN_HEIGHT-(270+R));
}


