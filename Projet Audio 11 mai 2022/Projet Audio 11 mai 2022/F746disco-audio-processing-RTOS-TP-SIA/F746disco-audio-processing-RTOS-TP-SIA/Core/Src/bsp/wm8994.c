/*
 * wm8994.c --- WM8994 audio codec driver
 *
 *  Created on: May 14, 2021
 *      Author: sydxrey
 */


#include "bsp/wm8994.h"
#include "bsp/disco_base.h"
#include "bsp/disco_i2c.h"
#include "stm32f7xx_hal.h"

/* Uncomment this line to enable verifying data sent to codec after each write
   operation (for debug purpose) */
#if !defined (VERIFY_WRITTENDATA)
/*#define VERIFY_WRITTENDATA*/
#endif /* VERIFY_WRITTENDATA */

static uint32_t outputEnabled = 0;
static uint32_t inputEnabled = 0;
static uint8_t ColdStartup = 1;

static uint8_t CODEC_IO_Write(uint16_t Reg, uint16_t Value);

/**
  * @brief Initializes the audio codec and the control interface.
  * @param OutputInputDevice: can be OUTPUT_DEVICE_SPEAKER, OUTPUT_DEVICE_HEADPHONE,
  *  OUTPUT_DEVICE_BOTH, OUTPUT_DEVICE_AUTO, INPUT_DEVICE_DIGITAL_MICROPHONE_1,
  *  INPUT_DEVICE_DIGITAL_MICROPHONE_2, INPUT_DEVICE_DIGITAL_MIC1_MIC2,
  *  INPUT_DEVICE_INPUT_LINE_1 or INPUT_DEVICE_INPUT_LINE_2.
  * @param Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param AudioFreq: Audio Frequency
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Init(uint16_t OutputInputDevice, uint8_t Volume, uint32_t AudioFreq)
{
  uint32_t counter = 0;
  uint16_t output_device = OutputInputDevice & 0xFF;
  uint16_t input_device = OutputInputDevice & 0xFF00;
  uint16_t power_mgnt_reg_1 = 0;

  /* Initialize the Control interface of the Audio Codec */
  //AUDIO_IO_Init(); // done in main.c

  /* wm8994 Errata Work-Arounds */
  counter += CODEC_IO_Write(0x102, 0x0003);
  counter += CODEC_IO_Write(0x817, 0x0000);
  counter += CODEC_IO_Write(0x102, 0x0000);

  /* Enable VMID soft start (fast), Start-up Bias Current Enabled */
  counter += CODEC_IO_Write(0x39, 0x006C);

    /* Enable bias generator, Enable VMID */
  if (input_device > 0)
  {
    counter += CODEC_IO_Write(0x01, 0x0013);
  }
  else
  {
    counter += CODEC_IO_Write(0x01, 0x0003);
  }

  /* Add Delay */
  HAL_Delay(50);

  /* Path Configurations for output */
  if (output_device > 0)
  {
    outputEnabled = 1;

    switch (output_device)
    {
    case OUTPUT_DEVICE_SPEAKER:
      /* Enable DAC1 (Left), Enable DAC1 (Right),
      Disable DAC2 (Left), Disable DAC2 (Right)*/
      counter += CODEC_IO_Write(0x05, 0x0C0C);

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x601, 0x0000);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x602, 0x0000);

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      counter += CODEC_IO_Write(0x604, 0x0002);

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      counter += CODEC_IO_Write(0x605, 0x0002);
      break;

    case OUTPUT_DEVICE_HEADPHONE:
      /* Disable DAC1 (Left), Disable DAC1 (Right),
      Enable DAC2 (Left), Enable DAC2 (Right)*/
      counter += CODEC_IO_Write(0x05, 0x0303);

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x601, 0x0001);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x602, 0x0001);

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      counter += CODEC_IO_Write(0x604, 0x0000);

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      counter += CODEC_IO_Write(0x605, 0x0000);
      break;

    case OUTPUT_DEVICE_BOTH:
      if (input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
      {
        /* Enable DAC1 (Left), Enable DAC1 (Right),
        also Enable DAC2 (Left), Enable DAC2 (Right)*/
        counter += CODEC_IO_Write(0x05, 0x0303 | 0x0C0C);

        /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path
        Enable the AIF1 Timeslot 1 (Left) to DAC 1 (Left) mixer path */
        counter += CODEC_IO_Write(0x601, 0x0003);

        /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path
        Enable the AIF1 Timeslot 1 (Right) to DAC 1 (Right) mixer path */
        counter += CODEC_IO_Write(0x602, 0x0003);

        /* Enable the AIF1 Timeslot 0 (Left) to DAC 2 (Left) mixer path
        Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path  */
        counter += CODEC_IO_Write(0x604, 0x0003);

        /* Enable the AIF1 Timeslot 0 (Right) to DAC 2 (Right) mixer path
        Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
        counter += CODEC_IO_Write(0x605, 0x0003);
      }
      else
      {
        /* Enable DAC1 (Left), Enable DAC1 (Right),
        also Enable DAC2 (Left), Enable DAC2 (Right)*/
        counter += CODEC_IO_Write(0x05, 0x0303 | 0x0C0C);

        /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
        counter += CODEC_IO_Write(0x601, 0x0001);

        /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
        counter += CODEC_IO_Write(0x602, 0x0001);

        /* Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
        counter += CODEC_IO_Write(0x604, 0x0002);

        /* Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
        counter += CODEC_IO_Write(0x605, 0x0002);
      }
      break;

    case OUTPUT_DEVICE_AUTO :
    default:
      /* Disable DAC1 (Left), Disable DAC1 (Right),
      Enable DAC2 (Left), Enable DAC2 (Right)*/
      counter += CODEC_IO_Write(0x05, 0x0303);

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x601, 0x0001);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x602, 0x0001);

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      counter += CODEC_IO_Write(0x604, 0x0000);

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      counter += CODEC_IO_Write(0x605, 0x0000);
      break;
    }
  }
  else
  {
    outputEnabled = 0;
  }

  /* Path Configurations for input */
  if (input_device > 0)
  {
    inputEnabled = 1;
    switch (input_device)
    {
    case INPUT_DEVICE_DIGITAL_MICROPHONE_2 :
      /* Enable AIF1ADC2 (Left), Enable AIF1ADC2 (Right)
       * Enable DMICDAT2 (Left), Enable DMICDAT2 (Right)
       * Enable Left ADC, Enable Right ADC */
      counter += CODEC_IO_Write(0x04, 0x0C30);

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC2 Left/Right Timeslot 1 */
      counter += CODEC_IO_Write(0x450, 0x00DB);

      /* Disable IN1L, IN1R, IN2L, IN2R, Enable Thermal sensor & shutdown */
      counter += CODEC_IO_Write(0x02, 0x6000);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x608, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x609, 0x0002);

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC2 signal detect */
      counter += CODEC_IO_Write(0x700, 0x000E);
      break;

    case INPUT_DEVICE_INPUT_LINE_1 :
      /* IN1LN_TO_IN1L, IN1LP_TO_VMID, IN1RN_TO_IN1R, IN1RP_TO_VMID */
      counter += CODEC_IO_Write(0x28, 0x0011);

      // !!! syd mai 2021 : bug fix for LINE 1 input gain was far too high,
      // see https://community.st.com/s/question/0D50X00009XkdfX/stm32f769idisco-audio-line-in

      /* Disable mute on IN1L_TO_MIXINL and +30dB on IN1L PGA output */
      // syd : counter += CODEC_IO_Write(0x29, 0x0035);
      /* CHANGE! MIXOUTR_MIXINR_VOL set to 0 (mute)
         +30db removed : 0x0020 instead of 0x0035  */
      counter += CODEC_IO_Write(0x29, 0x0020);

      /* Disable mute on IN1R_TO_MIXINL, Gain = +30dB */
      //syd : counter += CODEC_IO_Write(0x2A, 0x0035);
      /* CHANGE! MIXOUTL_MIXINL_VOL set to 0 (mute)
         +30db removed : 0x0020 instead of 0x0035  */
      counter += CODEC_IO_Write(0x2A, 0x0020);

      /* Enable AIF1ADC1 (Left), Enable AIF1ADC1 (Right)
       * Enable Left ADC, Enable Right ADC */
      counter += CODEC_IO_Write(0x04, 0x0303);

      /* Enable AIF1 DRC1 Signal Detect & DRC in AIF1ADC1 Left/Right Timeslot 0 */
      counter += CODEC_IO_Write(0x440, 0x00DB);

      /* Enable IN1L and IN1R, Disable IN2L and IN2R, Enable Thermal sensor & shutdown */
      counter += CODEC_IO_Write(0x02, 0x6350);

      /* Enable the ADCL(Left) to AIF1 Timeslot 0 (Left) mixer path */
      counter += CODEC_IO_Write(0x606, 0x0002);

      /* Enable the ADCR(Right) to AIF1 Timeslot 0 (Right) mixer path */
      counter += CODEC_IO_Write(0x607, 0x0002);

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC1 signal detect */
      counter += CODEC_IO_Write(0x700, 0x000D);
      break;

    case INPUT_DEVICE_DIGITAL_MICROPHONE_1 :
      /* Enable AIF1ADC1 (Left), Enable AIF1ADC1 (Right)
       * Enable DMICDAT1 (Left), Enable DMICDAT1 (Right)
       * Enable Left ADC, Enable Right ADC */
      counter += CODEC_IO_Write(0x04, 0x030C);

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC1 Left/Right Timeslot 0 */
      counter += CODEC_IO_Write(0x440, 0x00DB);

      /* Disable IN1L, IN1R, IN2L, IN2R, Enable Thermal sensor & shutdown */
      counter += CODEC_IO_Write(0x02, 0x6350);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 0 (Left) mixer path */
      counter += CODEC_IO_Write(0x606, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 0 (Right) mixer path */
      counter += CODEC_IO_Write(0x607, 0x0002);

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC1 signal detect */
      counter += CODEC_IO_Write(0x700, 0x000D);
      break;
    case INPUT_DEVICE_DIGITAL_MIC1_MIC2 :
      /* Enable AIF1ADC1 (Left), Enable AIF1ADC1 (Right)
       * Enable DMICDAT1 (Left), Enable DMICDAT1 (Right)
       * Enable Left ADC, Enable Right ADC */
      counter += CODEC_IO_Write(0x04, 0x0F3C);

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC2 Left/Right Timeslot 1 */
      counter += CODEC_IO_Write(0x450, 0x00DB);

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC1 Left/Right Timeslot 0 */
      counter += CODEC_IO_Write(0x440, 0x00DB);

      /* Disable IN1L, IN1R, Enable IN2L, IN2R, Thermal sensor & shutdown */
      counter += CODEC_IO_Write(0x02, 0x63A0);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 0 (Left) mixer path */
      counter += CODEC_IO_Write(0x606, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 0 (Right) mixer path */
      counter += CODEC_IO_Write(0x607, 0x0002);

      /* Enable the DMIC2(Left) to AIF1 Timeslot 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x608, 0x0002);

      /* Enable the DMIC2(Right) to AIF1 Timeslot 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x609, 0x0002);

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC1 signal detect */
      counter += CODEC_IO_Write(0x700, 0x000D);
      break;
    case INPUT_DEVICE_INPUT_LINE_2 :
    default:
      /* Actually, no other input devices supported */
      counter++;
      break;
    }
  }
  else
  {
    inputEnabled = 0;
  }

  /*  Clock Configurations */
  switch (AudioFreq)
  {
  case  AUDIO_FREQUENCY_8K:
    /* AIF1 Sample Rate = 8 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0003);
    break;

  case  AUDIO_FREQUENCY_16K:
    /* AIF1 Sample Rate = 16 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0033);
    break;

  case  AUDIO_FREQUENCY_32K:
    /* AIF1 Sample Rate = 32 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0063);
    break;

  case  AUDIO_FREQUENCY_48K:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0083);
    break;

  case  AUDIO_FREQUENCY_96K:
    /* AIF1 Sample Rate = 96 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x00A3);
    break;

  case  AUDIO_FREQUENCY_11K:
    /* AIF1 Sample Rate = 11.025 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0013);
    break;

  case  AUDIO_FREQUENCY_22K:
    /* AIF1 Sample Rate = 22.050 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0043);
    break;

  case  AUDIO_FREQUENCY_44K:
    /* AIF1 Sample Rate = 44.1 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0073);
    break;

  default:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */
    counter += CODEC_IO_Write(0x210, 0x0083);
    break;
  }

  if(input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
  {
  /* AIF1 Word Length = 16-bits, AIF1 Format = DSP mode */
  counter += CODEC_IO_Write(0x300, 0x4018);
  }
  else
  {
  /* AIF1 Word Length = 16-bits, AIF1 Format = I2S (Default Register Value) */
  counter += CODEC_IO_Write(0x300, 0x4010);
  }

  /* slave mode */
  counter += CODEC_IO_Write(0x302, 0x0000);

  /* Enable the DSP processing clock for AIF1, Enable the core clock */
  counter += CODEC_IO_Write(0x208, 0x000A);

  /* Enable AIF1 Clock, AIF1 Clock Source = MCLK1 pin */
  counter += CODEC_IO_Write(0x200, 0x0001);

  if (output_device > 0)  /* Audio output selected */
  {
    if (output_device == OUTPUT_DEVICE_HEADPHONE)
    {
      /* Select DAC1 (Left) to Left Headphone Output PGA (HPOUT1LVOL) path */
      counter += CODEC_IO_Write(0x2D, 0x0100);

      /* Select DAC1 (Right) to Right Headphone Output PGA (HPOUT1RVOL) path */
      counter += CODEC_IO_Write(0x2E, 0x0100);

      /* Startup sequence for Headphone */
      if(ColdStartup)
      {
        counter += CODEC_IO_Write(0x110,0x8100);

        ColdStartup=0;
        /* Add Delay */
        HAL_Delay(300);
      }
      else /* Headphone Warm Start-Up */
      {
        counter += CODEC_IO_Write(0x110,0x8108);
        /* Add Delay */
        HAL_Delay(50);
      }

      /* Soft un-Mute the AIF1 Timeslot 0 DAC1 path L&R */
      counter += CODEC_IO_Write(0x420, 0x0000);
    }
    /* Analog Output Configuration */

    /* Enable SPKRVOL PGA, Enable SPKMIXR, Enable SPKLVOL PGA, Enable SPKMIXL */
    counter += CODEC_IO_Write(0x03, 0x0300);

    /* Left Speaker Mixer Volume = 0dB */
    counter += CODEC_IO_Write(0x22, 0x0000);

    /* Speaker output mode = Class D, Right Speaker Mixer Volume = 0dB ((0x23, 0x0100) = class AB)*/
    counter += CODEC_IO_Write(0x23, 0x0000);

    /* Unmute DAC2 (Left) to Left Speaker Mixer (SPKMIXL) path,
    Unmute DAC2 (Right) to Right Speaker Mixer (SPKMIXR) path */
    counter += CODEC_IO_Write(0x36, 0x0300);

    /* Enable bias generator, Enable VMID, Enable SPKOUTL, Enable SPKOUTR */
    counter += CODEC_IO_Write(0x01, 0x3003);

    /* Headphone/Speaker Enable */

    if (input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
    {
    /* Enable Class W, Class W Envelope Tracking = AIF1 Timeslots 0 and 1 */
    counter += CODEC_IO_Write(0x51, 0x0205);
    }
    else
    {
    /* Enable Class W, Class W Envelope Tracking = AIF1 Timeslot 0 */
    counter += CODEC_IO_Write(0x51, 0x0005);
    }

    /* Enable bias generator, Enable VMID, Enable HPOUT1 (Left) and Enable HPOUT1 (Right) input stages */
    /* idem for Speaker */
    power_mgnt_reg_1 |= 0x0303 | 0x3003;
    counter += CODEC_IO_Write(0x01, power_mgnt_reg_1);

    /* Enable HPOUT1 (Left) and HPOUT1 (Right) intermediate stages */
    counter += CODEC_IO_Write(0x60, 0x0022);

    /* Enable Charge Pump */
    counter += CODEC_IO_Write(0x4C, 0x9F25);

    /* Add Delay */
    HAL_Delay(15);

    /* Select DAC1 (Left) to Left Headphone Output PGA (HPOUT1LVOL) path */
    counter += CODEC_IO_Write(0x2D, 0x0001);

    /* Select DAC1 (Right) to Right Headphone Output PGA (HPOUT1RVOL) path */
    counter += CODEC_IO_Write(0x2E, 0x0001);

    /* Enable Left Output Mixer (MIXOUTL), Enable Right Output Mixer (MIXOUTR) */
    /* idem for SPKOUTL and SPKOUTR */
    counter += CODEC_IO_Write(0x03, 0x0030 | 0x0300);

    /* Enable DC Servo and trigger start-up mode on left and right channels */
    counter += CODEC_IO_Write(0x54, 0x0033);

    /* Add Delay */
    HAL_Delay(257);

    /* Enable HPOUT1 (Left) and HPOUT1 (Right) intermediate and output stages. Remove clamps */
    counter += CODEC_IO_Write(0x60, 0x00EE);

    /* Unmutes */

    /* Unmute DAC 1 (Left) */
    counter += CODEC_IO_Write(0x610, 0x00C0);

    /* Unmute DAC 1 (Right) */
    counter += CODEC_IO_Write(0x611, 0x00C0);

    /* Unmute the AIF1 Timeslot 0 DAC path */
    counter += CODEC_IO_Write(0x420, 0x0010);

    /* Unmute DAC 2 (Left) */
    counter += CODEC_IO_Write(0x612, 0x00C0);

    /* Unmute DAC 2 (Right) */
    counter += CODEC_IO_Write(0x613, 0x00C0);

    /* Unmute the AIF1 Timeslot 1 DAC2 path */
    counter += CODEC_IO_Write(0x422, 0x0010);

    /* Volume Control */
    wm8994_SetVolume(Volume);
  }

  if (input_device > 0) /* Audio input selected */
  {
    if ((input_device == INPUT_DEVICE_DIGITAL_MICROPHONE_1) || (input_device == INPUT_DEVICE_DIGITAL_MICROPHONE_2))
    {
      /* Enable Microphone bias 1 generator, Enable VMID */
      power_mgnt_reg_1 |= 0x0013;
      counter += CODEC_IO_Write(0x01, power_mgnt_reg_1);

      /* ADC oversample enable */
      counter += CODEC_IO_Write(0x620, 0x0002);

      /* AIF ADC2 HPF enable, HPF cut = voice mode 1 fc=127Hz at fs=8kHz */
      counter += CODEC_IO_Write(0x411, 0x3800);
    }
    else if(input_device == INPUT_DEVICE_DIGITAL_MIC1_MIC2)
    {
      /* Enable Microphone bias 1 generator, Enable VMID */
      power_mgnt_reg_1 |= 0x0013;
      counter += CODEC_IO_Write(0x01, power_mgnt_reg_1);

      /* ADC oversample enable */
      counter += CODEC_IO_Write(0x620, 0x0002);

      /* AIF ADC1 HPF enable, HPF cut = voice mode 1 fc=127Hz at fs=8kHz */
      counter += CODEC_IO_Write(0x410, 0x1800);

      /* AIF ADC2 HPF enable, HPF cut = voice mode 1 fc=127Hz at fs=8kHz */
      counter += CODEC_IO_Write(0x411, 0x1800);
    }
    else if ((input_device == INPUT_DEVICE_INPUT_LINE_1) || (input_device == INPUT_DEVICE_INPUT_LINE_2))
    {

      /* Disable mute on IN1L, IN1L Volume = +0dB */
      counter += CODEC_IO_Write(0x18, 0x000B);

      /* Disable mute on IN1R, IN1R Volume = +0dB */
      counter += CODEC_IO_Write(0x1A, 0x000B);

      /* AIF ADC1 HPF enable, HPF cut = hifi mode fc=4Hz at fs=48kHz */
      counter += CODEC_IO_Write(0x410, 0x1800);
    }
    /* Volume Control */
    wm8994_SetVolume(Volume);
  }
  /* Return communication control value */
  return counter;
}

/**
  * @brief  Get the WM8994 ID.
  * @retval The WM8994 ID
  */
uint32_t wm8994_ReadID()
{
  /* Initialize the Control interface of the Audio Codec */
  //AUDIO_IO_Init(); // done in main.c

  return ((uint32_t)AUDIO_I2C_Read(WM8994_CHIPID_ADDR));
}

/**
  * @brief Start the audio Codec play feature.
  * @note For this codec no Play options are required.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Play(uint16_t* pBuffer, uint16_t Size)
{
  uint32_t counter = 0;

  /* Resumes the audio file playing */
  /* Unmute the output first */
  counter += wm8994_SetMute(AUDIO_MUTE_OFF);

  return counter;
}

/**
  * @brief Pauses playing on the audio codec.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Pause()
{
  uint32_t errorCounter = 0;

  /* Pause the audio file playing */
  /* Mute the output first */
  errorCounter += wm8994_SetMute(AUDIO_MUTE_ON);

  /* Put the Codec in Power save mode */
  errorCounter += CODEC_IO_Write(0x02, 0x01);

  return errorCounter;
}

/**
  * @brief Resumes playing on the audio codec.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Resume()
{
  uint32_t errorCounter = 0;

  /* Resumes the audio file playing */
  /* Unmute the output first */
  errorCounter += wm8994_SetMute(AUDIO_MUTE_OFF);

  return errorCounter;
}

/**
  * @brief Stops audio Codec playing. It powers down the codec.
  * @param CodecPdwnMode: selects the  power down mode.
  *          - CODEC_PDWN_SW: only mutes the audio codec. When resuming from this
  *                           mode the codec keeps the previous initialization
  *                           (no need to re-Initialize the codec registers).
  *          - CODEC_PDWN_HW: Physically power down the codec. When resuming from this
  *                           mode, the codec is set to default configuration
  *                           (user should re-Initialize the codec in order to
  *                            play again the audio stream).
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Stop(uint32_t CodecPdwnMode)
{
  uint32_t errorCounter = 0;

  if (outputEnabled != 0)
  {
    /* Mute the output first */
    errorCounter += wm8994_SetMute(AUDIO_MUTE_ON);

    if (CodecPdwnMode == CODEC_PDWN_SW)
    {
      /* Only output mute required*/
    }
    else /* CODEC_PDWN_HW */
    {
      /* Mute the AIF1 Timeslot 0 DAC1 path */
      errorCounter += CODEC_IO_Write(0x420, 0x0200);

      /* Mute the AIF1 Timeslot 1 DAC2 path */
      errorCounter += CODEC_IO_Write(0x422, 0x0200);

      /* Disable DAC1L_TO_HPOUT1L */
      errorCounter += CODEC_IO_Write(0x2D, 0x0000);

      /* Disable DAC1R_TO_HPOUT1R */
      errorCounter += CODEC_IO_Write(0x2E, 0x0000);

      /* Disable DAC1 and DAC2 */
      errorCounter += CODEC_IO_Write(0x05, 0x0000);

      /* Reset Codec by writing in 0x0000 address register */
      errorCounter += CODEC_IO_Write(0x0000, 0x0000);

      outputEnabled = 0;
    }
  }
  return errorCounter;
}

/**
  * @brief Sets higher or lower the codec volume level.
  * @param Volume: a byte value from 0 to 255 (refer to codec registers
  *         description for more details).
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_SetVolume(uint8_t Volume)
{
  uint32_t counter = 0;
  uint8_t convertedvol = VOLUME_CONVERT(Volume);

  /* Output volume */
  if (outputEnabled != 0)
  {
    if(convertedvol > 0x3E)
    {
      /* Unmute audio codec */
      counter += wm8994_SetMute(AUDIO_MUTE_OFF);

      /* Left Headphone Volume */
      counter += CODEC_IO_Write(0x1C, 0x3F | 0x140);

      /* Right Headphone Volume */
      counter += CODEC_IO_Write(0x1D, 0x3F | 0x140);

      /* Left Speaker Volume */
      counter += CODEC_IO_Write(0x26, 0x3F | 0x140);

      /* Right Speaker Volume */
      counter += CODEC_IO_Write(0x27, 0x3F | 0x140);
    }
    else if (Volume == 0)
    {
      /* Mute audio codec */
      counter += wm8994_SetMute(AUDIO_MUTE_ON);
    }
    else
    {
      /* Unmute audio codec */
      counter += wm8994_SetMute(AUDIO_MUTE_OFF);

      /* Left Headphone Volume */
      counter += CODEC_IO_Write(0x1C, convertedvol | 0x140);

      /* Right Headphone Volume */
      counter += CODEC_IO_Write(0x1D, convertedvol | 0x140);

      /* Left Speaker Volume */
      counter += CODEC_IO_Write(0x26, convertedvol | 0x140);

      /* Right Speaker Volume */
      counter += CODEC_IO_Write(0x27, convertedvol | 0x140);
    }
  }

  /* Input volume */
  if (inputEnabled != 0)
  {
    convertedvol = VOLUME_IN_CONVERT(Volume);

    /* Left AIF1 ADC1 volume */
    counter += CODEC_IO_Write(0x400, convertedvol | 0x100);

    /* Right AIF1 ADC1 volume */
    counter += CODEC_IO_Write(0x401, convertedvol | 0x100);

    /* Left AIF1 ADC2 volume */
    counter += CODEC_IO_Write(0x404, convertedvol | 0x100);

    /* Right AIF1 ADC2 volume */
    counter += CODEC_IO_Write(0x405, convertedvol | 0x100);
  }
  return counter;
}

/**
  * @brief Enables or disables the mute feature on the audio codec.
  * @param Cmd: AUDIO_MUTE_ON to enable the mute or AUDIO_MUTE_OFF to disable the
  *             mute mode.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_SetMute(uint32_t Cmd)
{
  uint32_t counter = 0;

  if (outputEnabled != 0)
  {
    /* Set the Mute mode */
    if(Cmd == AUDIO_MUTE_ON)
    {
      /* Soft Mute the AIF1 Timeslot 0 DAC1 path L&R */
      counter += CODEC_IO_Write(0x420, 0x0200);

      /* Soft Mute the AIF1 Timeslot 1 DAC2 path L&R */
      counter += CODEC_IO_Write(0x422, 0x0200);
    }
    else /* AUDIO_MUTE_OFF Disable the Mute */
    {
      /* Unmute the AIF1 Timeslot 0 DAC1 path L&R */
      counter += CODEC_IO_Write(0x420, 0x0010);

      /* Unmute the AIF1 Timeslot 1 DAC2 path L&R */
      counter += CODEC_IO_Write(0x422, 0x0010);
    }
  }
  return counter;
}

uint32_t wm8994_Mute(){

	wm8994_SetMute(AUDIO_MUTE_ON);

}

uint32_t wm8994_Unmute(){

	wm8994_SetMute(AUDIO_MUTE_OFF);

}

/**
  * @brief Switch dynamically (while audio file is played) the output target
  *         (speaker or headphone).
  * @param Output: specifies the audio output target: OUTPUT_DEVICE_SPEAKER,
  *         OUTPUT_DEVICE_HEADPHONE, OUTPUT_DEVICE_BOTH or OUTPUT_DEVICE_AUTO
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_SetOutputMode(uint8_t Output)
{
  uint32_t errorCounter = 0;

  switch (Output)
  {
  case OUTPUT_DEVICE_SPEAKER:
    /* Enable DAC1 (Left), Enable DAC1 (Right),
    Disable DAC2 (Left), Disable DAC2 (Right)*/
    errorCounter += CODEC_IO_Write(0x05, 0x0C0C);

    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x601, 0x0000);

    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x602, 0x0000);

    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x604, 0x0002);

    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x605, 0x0002);
    break;

  case OUTPUT_DEVICE_HEADPHONE:
    /* Disable DAC1 (Left), Disable DAC1 (Right),
    Enable DAC2 (Left), Enable DAC2 (Right)*/
    errorCounter += CODEC_IO_Write(0x05, 0x0303);

    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x601, 0x0001);

    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x602, 0x0001);

    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x604, 0x0000);

    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x605, 0x0000);
    break;

  case OUTPUT_DEVICE_BOTH:
    /* Enable DAC1 (Left), Enable DAC1 (Right),
    also Enable DAC2 (Left), Enable DAC2 (Right)*/
    errorCounter += CODEC_IO_Write(0x05, 0x0303 | 0x0C0C);

    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x601, 0x0001);

    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x602, 0x0001);

    /* Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x604, 0x0002);

    /* Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x605, 0x0002);
    break;

  default:
    /* Disable DAC1 (Left), Disable DAC1 (Right),
    Enable DAC2 (Left), Enable DAC2 (Right)*/
    errorCounter += CODEC_IO_Write(0x05, 0x0303);

    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x601, 0x0001);

    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x602, 0x0001);

    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    errorCounter += CODEC_IO_Write(0x604, 0x0000);

    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    errorCounter += CODEC_IO_Write(0x605, 0x0000);
    break;
  }
  return errorCounter;
}

/**
  * @brief Sets new frequency.
  * @param AudioFreq: Audio frequency used to play the audio stream.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_SetFrequency(uint32_t AudioFreq)
{
  uint32_t errorCounter = 0;

  /*  Clock Configurations */
  switch (AudioFreq)
  {
  case  AUDIO_FREQUENCY_8K:
    /* AIF1 Sample Rate = 8 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0003);
    break;

  case  AUDIO_FREQUENCY_16K:
    /* AIF1 Sample Rate = 16 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0033);
    break;

  case  AUDIO_FREQUENCY_32K:
    /* AIF1 Sample Rate = 32 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0063);
    break;

  case  AUDIO_FREQUENCY_48K:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0083);
    break;

  case  AUDIO_FREQUENCY_96K:
    /* AIF1 Sample Rate = 96 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x00A3);
    break;

  case  AUDIO_FREQUENCY_11K:
    /* AIF1 Sample Rate = 11.025 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0013);
    break;

  case  AUDIO_FREQUENCY_22K:
    /* AIF1 Sample Rate = 22.050 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0043);
    break;

  case  AUDIO_FREQUENCY_44K:
    /* AIF1 Sample Rate = 44.1 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0073);
    break;

  default:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */
    errorCounter += CODEC_IO_Write(0x210, 0x0083);
    break;
  }
  return errorCounter;
}

/**
  * @brief Resets wm8994 registers.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Reset()
{
  uint32_t counter = 0;

  /* Reset Codec by writing in 0x0000 address register */
  counter = CODEC_IO_Write(0x0000, 0x0000);
  outputEnabled = 0;
  inputEnabled=0;

  return counter;
}

/**
  * @brief  Writes/Read a single data to the onboard WM8994 audio Codec
  * @param  Reg: Reg address
  * @param  Value: Data to be written
  * @retval None
  */
static uint8_t CODEC_IO_Write(uint16_t Reg, uint16_t Value){

  AUDIO_I2C_Write(Reg, Value);

#ifdef VERIFY_WRITTENDATA
  /* Verify that the data has been correctly written */
  return (AUDIO_I2C_Read(Reg) == Value)? 0:1;
#else
  return 0;
#endif /* VERIFY_WRITTENDATA */
}

