//
// MAIN.C
// Sets up ADC1 channel 15 input on PC.5
//     and DAC2 output PA.5
//
// Also configures PC.4 as a GPIO and generates a 880Hz square wave
// Connect PC.4 to PC.5 to test system
//
// Written by Ross Wolin
//


#include <stdbool.h>
#include <string.h>

#include "stm32f4xx.h"

#include "hw.h"
#include "dac.h"
#include "adc.h"
#include "gpio_square_wave.h"
#include "tmr_sample.h"
#include "timer.h"
#include "led.h"



#define LED_PERIOD_MS      250


//Local functions
float filter(register float val);




int main(void)
{
	if (SysTick_Config(SystemCoreClock/1000)) {
	 	while (true)  // Capture error
          ;
	}

   led_init(LED1);   //PC2, used to show main loop (below) is running

   gsw_init();
   
   ADC_init();
   DAC2_init();
   tmr_sample_init();
   
   /*!< At this stage the microcontroller clock setting is already configured, 
     this is done through SystemInit() function which is called from startup
     file (startup_stm32f4xx.s) before to branch to application main.
     To reconfigure the default setting of SystemInit() function, refer to
     system_stm32f4xx.c file
   */    

   TMR tLED;
   tmr_setInterval(&tLED, LED_PERIOD_MS);

   while (true) {
      //Flash LED twice a second
      if (tmr_isExpired(&tLED)) {
         tmr_reset(&tLED);
         led_toggle(LED1);
      }
   }
}




//Time to load the DACs!
//This interrupt routine is called from a timer interrupt, at a rate of 44Khz,
//which is a good sampling rate for audio
//(Although the default handler has 'DAC' in the name, we are just using this
// as generic timer interrupt)
void TIM6_DAC_IRQHandler(void)
{
   if (TIM_GetITStatus(TIM6, TIM_IT_Update)) {

      //Process the ADC and DACs ...

      // Generate a square wave of approimately 880hz
      static int ctCycles=0;

      if (++ctCycles >= SAMPLE_FREQ/880/2) {
         ctCycles = 0;
         gsw_toggle();
      }

         
      int n = ADC_get();
      ADC_start();         //Start a new conversion

      //Write filtered waveform to DAC
      // (Notch filter removes the DC offset in the original waveform,
      //  so we add it back in)
      DAC2_set((uint16_t)(DAC_MID + (int)filter(n)));
         

      TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
   }
}






//Simple "bandpass resonator" (notch) filter I generated with FIVIEW,
// used to demostrate that the ADC to DAC path is working.

// Center frequency is 880Hz, Q=50
// FIVIEW generates code with doubles, I converted it to floats as
// the STM32F4 had hardware FP for floats, but not doubles

///////////////////////////////////////////////////////////////////////

// Generated by Fiview 0.9.10 <http://uazu.net/fiview/>.  
// All generated example code below is in the public domain.
// Filter 1
// File: -i #1
// Guessed type: band-pass
//
// Frequency-response:
//   Peak gain: 796.775
//   Guessed 100% gain: 796.775
//   Regions between half-power points (70.71% response or -3.01dB):
//     871.245Hz -> 888.845Hz  (width 17.6Hz, midpoint 880.045Hz)
//   Regions between quarter-power points (50% response or -6.02dB):
//     864.89Hz -> 895.372Hz  (width 30.482Hz, midpoint 880.131Hz)
//
// Filter descriptions:
//   BpRe/50/880 == Bandpass resonator, Q=50 (0 means Inf), frequency 880
//
// Example code (optimised for cleaner compilation to efficient machine code)
float filter(register float val)
{
   static float buf[2];
   register float tmp, fir, iir;
   tmp= buf[0]; memmove(buf, buf+1, 1*sizeof(float));

   iir= val * 0.001255059246835381 * 0.75;   
   iir -= 0.9974898815063291*tmp; fir= -tmp;
   iir -= -1.981739077169366*buf[0];
   
   fir += iir;
   buf[1]= iir; val= fir;
   return val;
}
