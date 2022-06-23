/*
 * audio_processing.c
 *
 *  Created on: May 17, 2021
 *      Authors: S. REYNAL, UI "améliorée" par Quentin Juarez et Paul Brumaire (SIA 2022)
 *
 *
 * === Audio latency ===
 *
 * Receive DMA copies audio samples from the CODEC to the DMA buffer (through the I2S serial bus) as interleaved stereo samples
 * (either slots 0 and 2 for the violet on-board "LineIn" connector, or slots 1 and 3, for the pair of on-boards microphones).
 *
 * Transmit DMA copies audio samples from the DMA buffer to the CODEC again as interleaved stereo samples (in the present
 * implementation only copying to the headphone output, that is, to slots 0 and 2, is available).
 *
 * For both input and output transfers, audio double-buffering is simply implemented by using
 * a large (receive or transmit) buffer of size AUDIO_DMA_BUF_SIZE
 * and implementing half-buffer and full-buffer DMA callbacks:
 * - HAL_SAI_RxHalfCpltCallback() gets called whenever the receive (=input) buffer is half-full
 * - HAL_SAI_RxCpltCallback() gets called whenever the receive (=input) buffer is full
 *
 * As a result, one audio frame has a size of AUDIO_DMA_BUF_SIZE/2. But since one audio frame
 * contains interleaved L and R stereo samples, its true duration is AUDIO_DMA_BUF_SIZE/4.
 *
 * Example:
 * 		AUDIO_BUF_SIZE = 512 (=size of a stereo audio frame)
 * 		AUDIO_DMA_BUF_SIZE = 1024 (=size of the whole DMA buffer)
 * 		The duration of ONE audio frame is given by AUDIO_BUF_SIZE/2 = 256 samples, that is, 5.3ms at 48kHz.
 *
 * === interprocess communication ===
 *
 *  Communication b/w DMA IRQ Handlers and the main audio loop is carried out
 *  using the "audio_rec_buffer_state" global variable (using the input buffer instead of the output
 *  buffer is a matter of pure convenience, as both are filled at the same pace anyway).
 *
 *  This variable can take on three possible values:
 *  - BUFFER_OFFSET_NONE: initial buffer state at start-up, or buffer has just been transferred to/from DMA
 *  - BUFFER_OFFSET_HALF: first-half of the DMA buffer has just been filled
 *  - BUFFER_OFFSET_FULL: second-half of the DMA buffer has just been filled
 *
 *  The variable is written by HAL_SAI_RxHalfCpltCallback() and HAL_SAI_RxCpltCallback() audio in DMA transfer callbacks.
 *  It is read inside the main audio loop (see audioLoop()).
 *
 *  If RTOS is to used, Signals may be used to communicate between the DMA IRQ Handler and the main audio loop audioloop().
 *
 */

#include <audio.h>
#include <ui.h>
#include <test.h>
#include <main.h>
#include <stdio.h>
#include "string.h"
#include "math.h"
#include "bsp/disco_sai.h"
#include "bsp/disco_base.h"
#include "cmsis_os.h"
#include "stdlib.h"
#include "arm_math.h"

extern SAI_HandleTypeDef hsai_BlockA2; // see main.c
extern SAI_HandleTypeDef hsai_BlockB2;
extern DMA_HandleTypeDef hdma_sai2_a;
extern DMA_HandleTypeDef hdma_sai2_b;

extern osThreadId defaultTaskHandle;
extern osThreadId uiTaskHandle;
extern delayMs;
extern int delayFeed;
extern int volume;
extern float depth;

// ---------- communication b/w DMA IRQ Handlers and the main while loop -------------

typedef enum {
	BUFFER_OFFSET_NONE = 0, BUFFER_OFFSET_HALF = 1, BUFFER_OFFSET_FULL = 2,
} BUFFER_StateTypeDef;
uint32_t audio_rec_buffer_state;




// ---------- DMA buffers ------------

// whole sample count in an audio frame: (beware: as they are interleaved stereo samples, true audio frame duration is given by AUDIO_BUF_SIZE/2)
#define AUDIO_BUF_SIZE   ((uint32_t)512)
/* size of a full DMA buffer made up of two half-buffers (aka double-buffering) */
#define AUDIO_DMA_BUF_SIZE   (2 * AUDIO_BUF_SIZE)

// DMA buffers are in embedded RAM:
int16_t buf_input[AUDIO_DMA_BUF_SIZE];
int16_t buf_output[AUDIO_DMA_BUF_SIZE];
int16_t *buf_input_half = buf_input + AUDIO_DMA_BUF_SIZE / 2;
int16_t *buf_output_half = buf_output + AUDIO_DMA_BUF_SIZE / 2;

// ---------- FFT buffers ------------
#define FFTLength 256

float32_t FFTInput[FFTLength];
float32_t FFTOutput[FFTLength];
float32_t FFTOutputMag[FFTLength/2];


// ------------- scratch float buffer for long delays, reverbs or long impulse response FIR based on float implementations ---------

uint32_t scratch_offset = 0; // see doc in processAudio()
#define AUDIO_SCRATCH_SIZE   16000*10//AUDIO_SCRATCH_MAXSZ_WORDS



// ------------ Private Function Prototypes ------------

static void processAudio(int16_t*, int16_t*);
static void accumulateInputLevels();
static float readFromAudioScratch(int pos);
static void writeToAudioScratch(float val, int pos);


// ----------- Local vars ------------

static int count = 0; // debug
static int posScratch = 0;
static int fillScratch = 0;
static double inputLevelL = 0;
static double inputLevelR = 0;
double inputLevelLavr;
double inputLevelRavr;
//float depth = 0.5; en statique sans slider


arm_rfft_fast_instance_f32 FFTStruct;

// ----------- Functions ------------

/**
 * This is the main audio loop (aka infinite while loop) which is responsible for real time audio processing tasks:
 * - transferring recorded audio from the DMA buffer to buf_input[]
 * - processing audio samples and writing them to buf_output[]
 * - transferring processed samples back to the DMA buffer
 */
void audioLoop() {

	//uiDisplayBasic();

	/* Initialize SDRAM buffers */
	memset((int16_t*) AUDIO_SCRATCH_ADDR, 0, AUDIO_SCRATCH_SIZE * 2); // note that the size argument here always refers to bytes whatever the data type

	audio_rec_buffer_state = BUFFER_OFFSET_NONE;

	arm_rfft_fast_init_f32(&FFTStruct, FFTLength);

	// input device: INPUT_DEVICE_INPUT_LINE_1 or INPUT_DEVICE_DIGITAL_MICROPHONE_2 (not fully functional yet as you also need to change things in main.c:MX_SAI2_Init())
	// AudioFreq: AUDIO_FREQUENCY_48K, AUDIO_FREQUENCY_16K, etc (but also change accordingly hsai_BlockA2.Init.AudioFrequency in main.c, line 855)
	//start_Audio_Processing(buf_output, buf_input, AUDIO_DMA_BUF_SIZE, INPUT_DEVICE_DIGITAL_MICROPHONE_2, SAI_AUDIO_FREQUENCY_16K); // AUDIO_FREQUENCY_48K);
	start_Audio_Processing(buf_output, buf_input, AUDIO_DMA_BUF_SIZE, INPUT_DEVICE_DIGITAL_MICROPHONE_2, hsai_BlockA2.Init.AudioFrequency);

	/* main audio loop */
	while (1) {

		accumulateInputLevels();
		count++;
		if (count >= 20) {
			count = 0;
			inputLevelLavr = inputLevelL * 0.05;
			inputLevelRavr = inputLevelR * 0.05;
			//osSignalSet(uiTaskHandle, 0x0002);
			//uiDisplayInputLevel(inputLevelL, inputLevelR);
			inputLevelL = 0.;
			inputLevelR = 0.;
		}

		osSignalWait (0x0002, osWaitForever);
		/* Wait until first half block has been recorded */
		//while (audio_rec_buffer_state != BUFFER_OFFSET_HALF) {
		//	asm("NOP");
		//}
		//audio_rec_buffer_state = BUFFER_OFFSET_NONE;
		/* Copy recorded 1st half block */
		processAudio(buf_output, buf_input);

		osSignalWait (0x0001, osWaitForever);
		/* Wait until second half block has been recorded */
		//while (audio_rec_buffer_state != BUFFER_OFFSET_FULL) {
		//	asm("NOP");
		//}
		//audio_rec_buffer_state = BUFFER_OFFSET_NONE;
		/* Copy recorded 2nd half block */
		processAudio(buf_output_half, buf_input_half);

	}
}


/*
 * Update input levels from the last audio frame (see global variable inputLevelL and inputLevelR).
 * Reminder: audio samples are actually interleaved L/R samples,
 * with left channel samples at even positions,
 * and right channel samples at odd positions.
 */
static void accumulateInputLevels() {

	// Left channel:
	uint32_t lvl = 0;
	for (int i = 0; i < AUDIO_DMA_BUF_SIZE; i += 2) {
		int16_t v = (int16_t) buf_input[i];
		if (v > 0)
			lvl += v;
		else
			lvl -= v;
	}
	inputLevelL += (double) lvl / AUDIO_DMA_BUF_SIZE / (1 << 15);

	// Right channel:
	lvl = 0;
	for (int i = 1; i < AUDIO_DMA_BUF_SIZE; i += 2) {
		int16_t v = (int16_t) buf_input[i];
		if (v > 0)
			lvl += v;
		else
			lvl -= v;
	}
	inputLevelR += (double) lvl / AUDIO_DMA_BUF_SIZE / (1 << 15);

}

//----------------------------FFT---------------------------------------



// --------------------------- Callbacks implementation ---------------------------

/**
 * Audio IN DMA Transfer complete interrupt.
 */
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai) {
	//audio_rec_buffer_state = BUFFER_OFFSET_FULL;
	osSignalSet(defaultTaskHandle, 0x0001);
	return;
}

/**
 * Audio IN DMA Half Transfer complete interrupt.
 */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai) {
	//audio_rec_buffer_state = BUFFER_OFFSET_HALF;
	osSignalSet(defaultTaskHandle, 0x0002);
	return;
}

// --------------------------- Audio scratch buffer ---------------------------

/**
 * Read a sample from the audio scratch buffer in SDRAM at position "pos"
 */
static float readFromAudioScratch(int pos) {

	__IO float *pSdramAddress = (float*) AUDIO_SCRATCH_ADDR;
	pSdramAddress += pos;
	return *(__IO float*) pSdramAddress;

}

/**
 * Write the given value to the audio scratch buffer in SDRAM at position "pos"
 */
static void writeToAudioScratch(float val, int pos) {

	__IO float *pSdramAddress = (float*) AUDIO_SCRATCH_ADDR;
	pSdramAddress += pos;
	*(__IO float*) pSdramAddress = val;

}

/**
 * Write the given value to the audio scratch buffer in SDRAM at position "pos"
 */


/**
static void writeToAudioScratchINT16(int16_t val, int pos) {
	__IO int16_t *pSdramAddress = (int16_t*) AUDIO_SCRATCH_ADDR;
	pSdramAddress += pos;
	*(__IO int16_t*) pSdramAddress = val;
}

static int16_t readFromAudioScratchINT16(int16_t val, int pos) {
	__IO int16_t *pSdramAddress = (int16_t*) AUDIO_SCRATCH_ADDR;
		pSdramAddress += pos;
		return *(__IO int16_t*) pSdramAddress;
}

*/

// --------------------------- AUDIO ALGORITHMS ---------------------------

/**
 * This function is called every time an audio frame
 * has been filled by the DMA, that is,  AUDIO_BUF_SIZE samples
 * have just been transferred from the CODEC
 * (keep in mind that this number represents interleaved L and R samples,
 * hence the true corresponding duration of this audio frame is AUDIO_BUF_SIZE/2 divided by the sampling frequency).
 */

static void processAudio(int16_t *out, int16_t *in) {

	LED_On(); // for oscilloscope measurements...

	/* 16KHz -> 1000ms*16 = 1s*/
	int delay = (int) 16 * delayMs;

	for (int n = 0; n < AUDIO_BUF_SIZE; n++) {

		//modulo AUDIO_SCRATCH_SIZE
		if(posScratch>AUDIO_SCRATCH_SIZE-1){
			posScratch=0;
		}

		if(fillScratch<delay){
			writeToAudioScratch((float)in[n],posScratch);
			posScratch+=1;
			out[n] = (in[n]*volume)/100;//out[n] = (in[n]*volume*depth)/100;
			fillScratch+=1;
		}else{
			int j = posScratch-delay;

			if(j<0){
				j=j+AUDIO_SCRATCH_SIZE;
			}
			/* old:  */
			int16_t old = (int16_t) readFromAudioScratch(j);

			float new = in[n]+(float)old/100*(delayFeed);
			//		///////delay
			writeToAudioScratch((float)new,posScratch);
			posScratch+=1;
			out[n]= (new*volume*depth)/100; //out[n]= (volume*depth)/100;
			//float new = in[n]+(float)old/100*(delayFeed); //échantillons actuels + les anciens * le mélange (feedback)
			//writeToAudioScratch((float)new,posScratch);
			//posScratch+=1;
			//out[n]= (volume*depth)/100;
		}
	}
	/* array copy */
	for(int i=0;i<FFTLength;i++){
		FFTInput[i]=(float32_t) out[i*2]/32738;
	}
		/* fft -> dB fft */
	arm_rfft_fast_f32(&FFTStruct,FFTInput,FFTOutput,0);
	arm_cmplx_mag_f32(FFTOutput,FFTOutputMag,FFTLength/2);

	osSignalSet(uiTaskHandle, 0x0001);

	LED_Off();
}

////////////////Traitement de l'audio////////////////////////////////
/* 16KHz -> 1000ms*16 = 1s*/
/*int delay = (int) 16 * delayMs;

for (int n = 0; n < AUDIO_BUF_SIZE; n++) {

	//modulo AUDIO_SCRATCH_SIZE
	if(posScratch>AUDIO_SCRATCH_SIZE-1){
		posScratch=0;
	}

	if(fillScratch<delay){
		writeToAudioScratch((float)in[n],posScratch);
		posScratch+=1;
		//out[n] = (in[n]*volume*depth)/100;
		fillScratch+=1;
	}else{
		int j = posScratch-delay;

		if(j<0){
			j=j+AUDIO_SCRATCH_SIZE;
		}
		/* old:  */
		//int16_t old = (int16_t) readFromAudioScratch(j);

		///////delay
		//float new = in[n]+(float)old/100*(delayFeed); //échantillons actuels + les anciens * le mélange (feedback)
		//writeToAudioScratch((float)new,posScratch);
		//posScratch+=1;
		//out[n]= (volume*depth)/100;
	//}
//}
////////////////Traitement de l'audio////////////////////////////////

