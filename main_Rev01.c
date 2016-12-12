/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*
 * ======== main.c ========
 * "Simple Send"
 *
 * This example shows a very simplified way of sending.  This might be used in
 * an application where the main functionality doesn't change much if a USB
 * host is present or not.  It simply calls USBCDC_sendDataInBackground(); if no
 * host is present, it simply carries on.
 *
 * Simply build, run, and attach to a USB host.  Run a terminal application
 * and open the COM port associated with this device.  (In Windows, check the
 * Device Manager.)  The time should be displayed, every second.
 *
 * Note that this code is identical to example H9, except for one line of
 * code:  the call to USBCDC_sendDataInBackground() instead of
 * USBHID_sendDataInBackground().  This reflects the symmetry between writing
 * MSP430 code for CDC vs. HID-Datapipe.
 *
 *+----------------------------------------------------------------------------+
 * Please refer to the MSP430 USB API Stack Programmer's Guide, located
 * in the root directory of this installation for more details.
 * ---------------------------------------------------------------------------*/

#include <string.h>
#include "driverlib.h"

#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/usb.h"                     //USB-specific functions
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "USB_app/usbConstructs.h"


/*
 * NOTE: Modify hal.h to select a specific evaluation board and customize for
 * your own board.
 */
#include "hal.h"

// Function declarations
void convertTimeBinToASCII(uint8_t* str);
void convertDateBinToASCII(uint8_t* str);

void convertCountBinToASCII(uint8_t* str);
void initRTC(void);

// Application globals
volatile Calendar newTime;
volatile uint8_t hour = 4, min = 30, sec = 00;  // Real-time clock (RTC) values.  4:30:00
volatile uint8_t day, month;
volatile uint8_t Rx_buf[10];

typedef volatile union yeartype{
		uint16_t currentyear; //To save voltages on CH0

        struct {
			uint8_t LSB;
			uint8_t MSB;
        };
} yeartype;


yeartype year;

volatile uint8_t bSendTimeToHost = FALSE;       // RTC-->main():  "send the time over USB"
uint8_t timeStr[10];                    // Stores the time as an ASCII string
uint8_t dateStr[12];
uint8_t cntrStr[12];					// convertCountBinToASCII(cntrStr);

// Global flags set by events
volatile uint8_t bCDCDataReceived_event = FALSE;  // Flag set by event handler to
                                               // indicate data has been
                                               // received into USB buffer

#define BUFFER_SIZE 256
char dataBuffer[BUFFER_SIZE] = "";

/*
 * ======== main ========
 */
volatile uint32_t PulseCount = 0;
volatile uint8_t SecCount = 0;
volatile unsigned int i;
void main(void)
{
	Calendar currentTime;

	WDT_A_hold(WDT_A_BASE); //Stop watchdog timer

//Initialize GPIO
    //P2DIR |= BIT0;                // P2.0/LED output direction
	//Select CBOUT function on P1.6/CBOUT and set P1.6 to output direction
	GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN6);
	//Set P2.0 to output direction
	GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0);
    P2DIR |= BIT2;
    P7DIR |= BIT4;
//**************************************************************************************
//Real Time Clock: Initialize Calendar Mode of RTC
   // Select XT1
   GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN4 + GPIO_PIN5);

  //Initialize LFXT1
  UCS_turnOnLFXT1(UCS_XT1_DRIVE_3, UCS_XCAP_3);


  //Setup Current Date and Time for Calendar
  currentTime.Seconds = 0x00;
  currentTime.Minutes = 18;
  currentTime.Hours = 10;
  currentTime.DayOfWeek = 0x03;
  currentTime.DayOfMonth = 24;
  currentTime.Month = 10;
  currentTime.Year = 2016;

  /*
   * Base Address of the RTC_A_A
   * Pass in current time, intialized above
   * Use BCD as Calendar Register Format
   */
   //RTC_A_initCalendar(RTC_A_BASE, &currentTime, RTC_A_FORMAT_BCD);
   RTC_A_initCalendar(RTC_A_BASE, &currentTime, RTC_A_FORMAT_BINARY);

   //Setup Calendar Alarm for 5:00pm on the 5th day of the week.
   //Note: Does not specify day of the week.
   RTC_A_configureCalendarAlarmParam param = {0};
   param.minutesAlarm = 0x00;
   param.hoursAlarm = 0x17;
   param.dayOfWeekAlarm = RTC_A_ALARMCONDITION_OFF;
   param.dayOfMonthAlarm = 0x05;
   RTC_A_configureCalendarAlarm(RTC_A_BASE, &param);

   //Specify an interrupt to assert every minute
   RTC_A_setCalendarEvent(RTC_A_BASE, RTC_A_CALENDAREVENT_MINUTECHANGE);

   //Enable interrupt for RTC Ready Status, which asserts when the RTC
   //Calendar registers are ready to read.
   //Also, enable interrupts for the Calendar alarm and Calendar event.
   RTC_A_clearInterrupt(RTC_A_BASE, RTCRDYIFG + RTCTEVIFG + RTCAIFG);
   RTC_A_enableInterrupt(RTC_A_BASE, RTCRDYIE + RTCTEVIE + RTCAIE);

   //Start RTC Clock
   RTC_A_startClock(RTC_A_BASE);

   //Initialize the Comparator B module
   /* Base Address of Comparator B,
    * Pin CB0 to Positive(+) Terminal,
    * Reference Voltage to Negative(-) Terminal,
    * Normal Power Mode,
    * Output Filter On with minimal delay,
    * Non-Inverted Output Polarity
    */

//*************************************************************************************

//Set comparator B

   //Select CBOUT function on P1.6/CBOUT and set P1.6 to output direction
   //CBOUT will be "high" or "low" depending on the state of the comparator and CBEX setting
   GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN6);


    Comp_B_initParam param1 = {0};
    param1.positiveTerminalInput = COMP_B_INPUT0;
    param1.negativeTerminalInput = COMP_B_VREF;
    param1.powerModeSelect = COMP_B_POWERMODE_NORMALMODE;
    param1.outputFilterEnableAndDelayLevel = COMP_B_FILTEROUTPUT_DLYLVL1;
    param1.invertedOutputPolarity = COMP_B_NORMALOUTPUTPOLARITY;
    Comp_B_init(COMP_B_BASE, &param1);

    //Set the reference voltage that is being supplied to the (-) terminal
    /* Base Address of Comparator B,
     * Reference Voltage of 2.0 V,
     * Lower Limit of 2.0*(8/32) = 0.5V,
     * Upper Limit of 2.0*(8/32) = 0.5V,
     * Static Mode Accuracy
     */
     Comp_B_configureReferenceVoltageParam refVoltageParam = {0};
     refVoltageParam.supplyVoltageReferenceBase = COMP_B_VREFBASE2_0V;
     refVoltageParam.lowerLimitSupplyVoltageFractionOf32 = 8;
     refVoltageParam.upperLimitSupplyVoltageFractionOf32 = 8;
     refVoltageParam.referenceAccuracy = COMP_B_ACCURACY_STATIC;
     Comp_B_configureReferenceVoltage(COMP_B_BASE, &refVoltageParam);

     CBINT &= ~(CBIFG + CBIIFG);   // Clear any errant interrupts
     CBINT  |= CBIE;               // Enable CompB Interrupt on rising edge of CBIFG (CBIES=0)

     //Allow power to Comparator module
     //Comp_B_enable(COMP_B_BASE);
     //Comp_B_disable(COMP_B_BASE);

     //delay for the reference to settle
     __delay_cycles(75);

//*******************************************************************************************************

//USB initialize
    // Minimum Vcore setting required for the USB API is PMM_CORE_LEVEL_2
    PMM_setVCore(PMM_CORE_LEVEL_2);
    USBHAL_initPorts();           // Config GPIOS for low-power (output low)
    USBHAL_initClocks(8000000);   // Config clocks. MCLK=SMCLK=FLL=8MHz; ACLK=REFO=32kHz
    USB_setup(TRUE,TRUE);  // Init USB & events; if a host is present, connect

    initRTC();             // Start the real-time clock (counter). Used to upload current time from RTClock

    //__bis_SR_register(LPM0_bits + GIE + LPM4_bits);
    //Allow power to Comparator module
    Comp_B_enable(COMP_B_BASE);
    __no_operation();
    __no_operation();
    //Comp_B_disable(COMP_B_BASE);
    //__no_operation();

    //delay for the reference to settle
    __delay_cycles(75);

    __enable_interrupt();  // Enable interrupts globally

    while (1)
    {

    	uint8_t ReceiveError = 0, SendError = 0;
    	uint16_t count;

    	// Check the USB state and directly main loop accordingly
    	switch (USB_getConnectionState())
    	{
    		// This case is executed while your device is enumerated on the
    	    // USB host
    	    case ST_ENUM_ACTIVE:

    	    	// Sleep if there are no bytes to process.
    	    	__disable_interrupt();

    	    	if (!USBCDC_getBytesInUSBBuffer(CDC0_INTFNUM)) {
    	    		// Enter LPM0 until awakened by an event handler
    	    		__bis_SR_register(LPM0_bits + GIE);
    	    	}

    	    	__enable_interrupt();

    	    	// Exit LPM because of a data-receive event, and
    	    	// fetch the received data
    	    	if (bCDCDataReceived_event){

    	    		// Clear flag early -- just in case execution breaks
    	    		// below because of an error
    	    		bCDCDataReceived_event = FALSE;

    	    		count = USBCDC_receiveDataInBuffer((uint8_t*)dataBuffer,
    	    				BUFFER_SIZE,
							CDC0_INTFNUM);

    	    		// Count has the number of bytes received into dataBuffer
    	    		// Echo back to the host.
    	    		if (USBCDC_sendDataInBackground((uint8_t*)dataBuffer,
    	    				count, CDC0_INTFNUM, 1)){
    	    				// Exit if something went wrong.
    	    				SendError = 0x01;
    	    				break;
    	    		}

    	         }

    	    	// If USB is present, sent the time to the host.  Flag is set every sec
    	    	if (bSendTimeToHost)
    	    	{
    	    		bSendTimeToHost = FALSE;
    	    	    // This function begins the USB send operation, and immediately
    	    	    // returns, while the sending happens in the background.
    	    	    // Send timeStr, 9 bytes, to intf #0 (which is enumerated as a
    	    	    // COM port).  1000 retries.  (Retries will be attempted if the
    	    	    // previous send hasn't completed yet).  If the bus isn't present,
    	    	    // it simply returns and does nothing.
    	    	    //if (USBCDC_sendDataInBackground(timeStr, 9, CDC0_INTFNUM, 1000))
    	    		convertTimeBinToASCII(timeStr);			//uint8_t timeStr[9];
    	    	    if (USBCDC_sendDataInBackground(timeStr, 10, CDC0_INTFNUM, 1000))
    	    	    {
    	    	    	_NOP();  	// If it fails, it'll end up here.  Could happen if
    	    	    	    		// the cable was detached after the connectionState()
    	    	    }           	// check, or if somehow the retries failed

    	    	    convertDateBinToASCII(dateStr);
    	    	    //if (USBCDC_sendDataInBackground(dateStr, 9, CDC0_INTFNUM, 1000))
    	    	    if (USBCDC_sendDataInBackground(dateStr, 12, CDC0_INTFNUM, 1000))
    	    	    {
    	    	    	_NOP();  	// If it fails, it'll end up here.  Could happen if
    	    	    	    		// the cable was detached after the connectionState()
    	    	    }
    	    	    if (SecCount == 9){
    	    	    	convertCountBinToASCII(cntrStr);
    	    	    	if (USBCDC_sendDataInBackground(cntrStr, 12, CDC0_INTFNUM, 1000))
    	    	    	{
    	    	    		_NOP();  // If it fails, it'll end up here.  Could happen if
    	    	    	    		            	             // the cable was detached after the connectionState()
    	    	    	}
    	    	    	PulseCount = 0;
    	    	    	SecCount = 0;
    	    	    }

    	    	}
    	       break;

    	       // These cases are executed while your device is disconnected from
    	       // the host (meaning, not enumerated); enumerated but suspended
    	       // by the host, or connected to a powered hub without a USB host
    	       // present.
    	       case ST_PHYS_DISCONNECTED:
    	       case ST_ENUM_SUSPENDED:
    	       case ST_PHYS_CONNECTED_NOENUM_SUSP:
    	    	   __bis_SR_register(LPM3_bits + GIE);
    	           _NOP();
    	       break;

    	            // The default is executed for the momentary state
    	            // ST_ENUM_IN_PROGRESS.  Usually, this state only last a few
    	            // seconds.  Be sure not to enter LPM3 in this state; USB
    	            // communication is taking place here, and therefore the mode must
    	            // be LPM0 or active-CPU.
    	            case ST_ENUM_IN_PROGRESS:
    	            default:;
    	        }

    	        if (ReceiveError || SendError){
    	            // TO DO: User can place code here to handle error
    	        }

    		// Enter LPM0, which keeps the DCO/FLL active but shuts off the
    		// CPU.  For USB, you can't go below LPM0!
    		__bis_SR_register(LPM0_bits + GIE + LPM4_bits);

    }  //while(1)
}  //main()


// Starts a real-time clock on TimerA_0.  Earlier we assigned ACLK to be driven
// by the REFO, at 32768Hz.  So below we set the timer to count up to 32768 and
// roll over; and generate an interrupt when it rolls over.
void initRTC(void)
{
    TA0CCR0 = 32768;
    TA0CTL = TASSEL_1+MC_1+TACLR; // ACLK, count to CCR0 then roll, clear TAR
    TA0CCTL0 = CCIE;              // Gen int at rollover (TIMER0_A0 vector)
}

// Convert a number 'bin' of value 0-99 into its ASCII equivalent.  Assumes
// str is a two-byte array.
void convertTwoDigBinToASCII(uint8_t bin, uint8_t* str)
{
    str[0] = '0';
    if (bin >= 10)
    {
        str[0] = (bin / 10) + 48;
    }
    str[1] = (bin % 10) + 48;
}



/*
 * Convert uint32_t hex value to an uint8_t array. Use this function to convert year to character decimal string.
 */
void hexdec_long( uint32_t count )
{
    uint8_t ones;
    uint8_t tens;
    uint8_t hundreds;
    uint8_t thousands;
    uint8_t thousand10s;
    uint8_t thousand100s;
    uint8_t mill;
    uint8_t mill10s;
    uint8_t mill100s;
    uint8_t bills;

	bills			= 0;
	mill100s		= 0;
	mill10s			= 0;
	mill			= 0;
	thousand100s 	= 0;
	thousand10s 	= 0;
	thousands 		= 0;
	hundreds 		= 0;
	tens  			= 0;
	ones 			= 0;

	while ( count >= 1000000000 )
	{
		count -= 1000000000;		// subtract 1000000000, one billion
		bills++;					// increment billions
	}
	while ( count >= 100000000 )
	{
		count -= 100000000;			// subtract 100000000, 100 million
		mill100s++;					// increment 100th millions
	}
	while ( count >= 10000000 )
	{
		count -= 10000000;			// subtract 10000000
		mill10s++;					// increment 10th millions
	}
	while ( count >= 1000000 )
	{
		count -= 1000000;			// subtract 1000000
		mill++;						// increment 1 millions
	}
	while ( count >= 100000 )
	{
		count -= 100000;			// subtract 100000
		thousand100s++;				// increment 100th thousands
	}
	while ( count >= 10000 )
	{
		count -= 10000;             // subtract 10000
		thousand10s++;				// increment 10th thousands
	}
	while ( count >= 1000 )
	{
		count -= 1000;				// subtract 1000
		thousands++;				// increment thousands
	}
	while ( count >= 100 )
	{
		count -= 100;               // subtract 100
		hundreds++;                 // increment hundreds
	}
	while ( count >= 10 )
	{
		count -= 10;				// subtract 10
		tens++;						// increment tens
	}
		ones = count;				// remaining count equals ones

	Rx_buf[0]= bills + 0x30;    //Conver HEX to character
	Rx_buf[1]= mill100s + 0x30;

    Rx_buf[2]= mill10s + 0x30;
    Rx_buf[3]= mill + 0x30;
    Rx_buf[4]= thousand100s + 0x30;
    Rx_buf[5]= thousand10s + 0x30;

    Rx_buf[6]= thousands + 0x30;
    Rx_buf[7]= hundreds + 0x30;

    Rx_buf[8]= tens   + 0x30;
    Rx_buf[9]= ones + 0x30;
    return;
}


// Convert the binary globals hour/min/sec into a string, of format "hr:mn:sc"
// Assumes str is an nine-byte string.
void convertTimeBinToASCII(uint8_t* str)
{
    uint8_t hourStr[2], minStr[2], secStr[2];

    convertTwoDigBinToASCII(hour, hourStr);
    convertTwoDigBinToASCII(min, minStr);
    convertTwoDigBinToASCII(sec, secStr);

    str[0] = '\n';
    str[1] = hourStr[0];
    str[2] = hourStr[1];
    str[3] = ':';
    str[4] = minStr[0];
    str[5] = minStr[1];
    str[6] = ':';
    str[7] = secStr[0];
    str[8] = secStr[1];
    str[9] = '\n';
}



// Convert MONTH DATE YEAR
void convertDateBinToASCII(uint8_t* str)
{
    uint8_t dayStr[2], monthStr[2];


    convertTwoDigBinToASCII(day, dayStr);
    convertTwoDigBinToASCII(month, monthStr);
    //void hexdec_long( uint32_t count )
    hexdec_long((uint32_t) year.currentyear);

    str[0] 	= '\n';
    str[1] 	= dayStr[0];
    str[2] 	= dayStr[1];
    str[3] 	= ':';
    str[4] 	= monthStr[0];
    str[5] 	= monthStr[1];
    str[6] 	= ':';
    str[7] = Rx_buf[6];
    str[8] = Rx_buf[7];
    str[9] = Rx_buf[8];
    str[10] = Rx_buf[9];
    str[11] = '\n';

}


// Timer0 A0 interrupt service routine.  Generated when TimerA_0 (real-time clock)
// rolls over from 32768 to 0, every second.
#if defined(__TI_COMPILER_VERSION__) || (__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
#elif defined(__GNUC__) && (__MSP430__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMER0_A0_ISR (void)
#else
#error Compiler not found!
#endif
{
   // if (sec++ == 60)
   // {
   //     sec = 0;
   //     if (min++ == 60)
   //     {
   //         min = 0;
   //         if (hour++ == 24)
   //         {
   //             hour = 0;
   //         }
   //     }
   // }

    //bSendTimeToHost = TRUE;                 // Time to update
    //__bic_SR_register_on_exit(LPM3_bits);   // Exit LPM
}

/*
 * ======== UNMI_ISR ========
 */
#if defined(__TI_COMPILER_VERSION__) || (__IAR_SYSTEMS_ICC__)
#pragma vector = UNMI_VECTOR
__interrupt void UNMI_ISR (void)
#elif defined(__GNUC__) && (__MSP430__)
void __attribute__ ((interrupt(UNMI_VECTOR))) UNMI_ISR (void)
#else
#error Compiler not found!
#endif
{
        switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG )) {
        case SYSUNIV_NONE:
                __no_operation();
                break;
        case SYSUNIV_NMIIFG:
                __no_operation();
                break;
        case SYSUNIV_OFIFG:

                UCS_clearFaultFlag(UCS_XT2OFFG);
                UCS_clearFaultFlag(UCS_DCOFFG);
                SFR_clearInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);
                break;
        case SYSUNIV_ACCVIFG:
                __no_operation();
                break;
        case SYSUNIV_BUSIFG:
                // If the CPU accesses USB memory while the USB module is
                // suspended, a "bus error" can occur.  This generates an NMI.  If
                // USB is automatically disconnecting in your software, set a
                // breakpoint here and see if execution hits it.  See the
                // Programmer's Guide for more information.
                SYSBERRIV = 0;  // Clear bus error flag
                USB_disable();  // Disable
        }
}
// RTC (real time clock) Interrupt interrupt on every second ------------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(RTC_VECTOR)))
#endif
void RTC_A_ISR(void)
{
	newTime = RTC_A_getCalendarTime(RTC_A_BASE);
    switch(__even_in_range(RTCIV,16))
    {
    case 0: break;      //No interrupts
    case 2:             //RTCRDYIFG
        //Toggle P2.0 every second
        //GPIO_toggleOutputOnPin(GPIO_PORT_P2,GPIO_PIN0);
        sec = newTime.Seconds;
        hour = newTime.Hours;
        min = newTime.Minutes;
        day = newTime.DayOfMonth;
        month = newTime.Month;
        year.currentyear = newTime.Year;
        bSendTimeToHost = TRUE;					// Time to update, enable USB transfer
        __bic_SR_register_on_exit(LPM3_bits);   // Exit LPM
        SecCount++;
        //year.currentyear = currentTime.Year;
        break;
    case 4:             //RTCEVIFG
        //Interrupts every minute
        __no_operation();
        break;
    case 6:             //RTCAIFG
        //Interrupts 5:00pm on 5th day of week
        __no_operation();
        break;
    case 8: break;      //RT0PSIFG
    case 10: break;     //RT1PSIFG
    case 12: break;     //Reserved
    case 14: break;     //Reserved
    case 16: break;     //Reserved
    default: break;
    }
}


// Convert MONTH DATE YEAR
void convertCountBinToASCII(uint8_t* str)
{


	PulseCount = PulseCount /99;
    hexdec_long((uint32_t) PulseCount);

    str[0] 	= '\n';
    str[1] 	= Rx_buf[0];
    str[2] 	= Rx_buf[1];
    str[3] 	= Rx_buf[2];
    str[4] 	= Rx_buf[3];
    str[5] 	= Rx_buf[4];
    str[6] 	= Rx_buf[5];
    str[7] 	= Rx_buf[6];
    str[8] 	= Rx_buf[7];
    str[9] 	= Rx_buf[8];
    str[10] = Rx_buf[9];
    str[11] = '\n';

}



// Comp_B ISR - LED Toggle Interrupt --------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=COMP_B_VECTOR
__interrupt void Comp_B_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(COMP_B_VECTOR))) Comp_B_ISR (void)
#else
#error Compiler not supported!
#endif
{
  //CBCTL1 ^= CBIES;              // Toggles interrupt edge
  CBINT &= ~CBIFG;              // Clear Interrupt flag
  GPIO_toggleOutputOnPin(GPIO_PORT_P2,GPIO_PIN0);
  //if (PulseCount < 4095)
	  PulseCount++;

  //else{
	  //P2OUT ^= BIT2;
	  //PulseCount = 0;
  //}
 // __bic_SR_register_on_exit
  //__bis_SR_register_on_exit(LPM4_bits + GIE);
  //bSendTimeToHost = TRUE;                 // Time to update
 //__bic_SR_register_on_exit(LPM3_bits);   // Exit LPM
  //__bic_SR_register_on_exit(CPUOFF);
}
