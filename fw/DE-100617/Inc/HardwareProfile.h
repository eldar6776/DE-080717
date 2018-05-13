/********************************************************************
 FileName:     	HardwareProfile - PICDEM FSUSB K50.h
 Dependencies:  See INCLUDES section
 Processor:     PIC18 USB Microcontrollers
 Hardware:      PICDEM FSUSB populated with PIC18F45K50
 Compiler:      Microchip C18
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC� Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

 ********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  2.9f  06/25/2012   Initial release
 ********************************************************************/

#ifndef HARDWARE_PROFILE_PICDEM_FSUSB_K50_H
#define HARDWARE_PROFILE_PICDEM_FSUSB_K50_H

/*******************************************************************/
/******** USB stack hardware selection options *********************/
/*******************************************************************/
//This section is the set of definitions required by the MCHPFSUSB
//  framework.  These definitions tell the firmware what mode it is
//  running in, and where it can find the results to some information
//  that the stack needs.
//These definitions are required by every application developed with
//  this revision of the MCHPFSUSB framework.  Please review each
//  option carefully and determine which options are desired/required
//  for your application.

//The PICDEM FS USB Demo Board platform supports the USE_SELF_POWER_SENSE_IO
//and USE_USB_BUS_SENSE_IO features.  Uncomment the below line(s) if
//it is desireable to use one or both of the features.
//#define USE_SELF_POWER_SENSE_IO
//#define tris_self_power     TRISAbits.TRISA2    // Input
#if defined(USE_SELF_POWER_SENSE_IO)
#define self_power          PORTAbits.RA2
#else
#define self_power          1
#endif

//#define USE_USB_BUS_SENSE_IO
//#define tris_usb_bus_sense  TRISAbits.TRISA1    // Input
#if defined(USE_USB_BUS_SENSE_IO)
#define USB_BUS_SENSE       PORTAbits.RA1
#else
#define USB_BUS_SENSE       1
#endif


//Uncomment the following line to make the output HEX of this  
//  project work with the MCHPUSB Bootloader    
//#define PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER

//Uncomment the following line to make the output HEX of this 
//  project work with the HID Bootloader
#define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER		

/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/******** Application specific definitions *************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/

/** Board definition ***********************************************/
//These defintions will tell the main() function which board is
//  currently selected.  This will allow the application to add
//  the correct configuration bits as wells use the correct
//  initialization functions for the board.  These defitions are only
//  required in the stack provided demos.  They are not required in
//  final application design.
#define DEMO_BOARD PICDEM_FS_USB_K50
#define PICDEM_FS_USB_K50
#define PIC18F45K50_FAMILY
#define CLOCK_FREQ 48000000


/** LED ************************************************************/
//#define LED_RED_DOUT            (LATAbits.LATA5)
//#define LED_GREEN_DOUT          (LATAbits.LATA4)
//#define BUZZER_DOUT             (LATCbits.LATC1)
//#define POWER_CONTROL_DOUT      (LATCbits.LATC6)

#define LED_BLUE_DOUT           (LATAbits.LATA4)
#define LED_RED_DOUT            (LATAbits.LATA7)
#define BUZZER_DOUT             (LATCbits.LATC1)

#define RedLed_On()             (LED_RED_DOUT = 0)
#define RedLed_Off()            (LED_RED_DOUT = 1)
#define RedLed_Toggle()         (LED_RED_DOUT = !LED_RED_DOUT)
#define GetRedLedState()        (LED_RED_DOUT)

//#define GreenLed_On()           (LED_GREEN_DOUT = 0)
//#define GreenLed_Off()          (LED_GREEN_DOUT = 1)
//#define GreenLed_Toggle()       (LED_GREEN_DOUT = !LED_GREEN_DOUT)
//#define GetGreenLedState()      (LED_GREEN_DOUT)

#define BlueLed_On()            (LED_BLUE_DOUT = 1)
#define BlueLed_Off()           (LED_BLUE_DOUT = 0)
#define BlueLed_Toggle()        (LED_BLUE_DOUT = !LED_BLUE_DOUT)
#define GetBlueLedState()       (LED_BLUE_DOUT)

#define Buzzer_On()             (BUZZER_DOUT = 1)
#define Buzzer_Off()            (BUZZER_DOUT = 0)
#define Buzzer_Toggle()         (BUZZER_DOUT = !BUZZER_DOUT)
#define GetBuzzerState()        (BUZZER_DOUT)

//#define PowerControl_On()       (POWER_CONTROL_DOUT = 0)
//#define PowerControl_Off()      (POWER_CONTROL_DOUT = 1)
//#define PowerControl_Toggle()   (POWER_CONTROL_DOUT = !POWER_CONTROL_DOUT)
//#define GetPowerControlState()  (POWER_CONTROL_DOUT)


//#define mInitAllLEDs()      LATA &= 0x0f; TRISA &= 0x0f; LATC &= 0x01; TRISC &= 0xfe;
//
//#define mLED_1              LATAbits.LATA6
//#define mLED_2              //LATAbits.LATA6
//#define mLED_3              //LATCbits.LATC0
//
//#define mGetLED_1()         mLED_1
//#define mGetLED_2()         mLED_2
//#define mGetLED_3()         mLED_3
//
//#define mLED_1_On()         mLED_1 = 1;
//#define mLED_2_On()         mLED_2 = 1;
//#define mLED_3_On()         mLED_3 = 1;
//
//#define mLED_1_Off()        mLED_1 = 0;
//#define mLED_2_Off()        mLED_2 = 0;
//#define mLED_3_Off()        mLED_3 = 0;
//
//#define mLED_1_Toggle()     mLED_1 = !mLED_1;
//#define mLED_2_Toggle()     mLED_2 = !mLED_2;
//#define mLED_3_Toggle()     mLED_3 = !mLED_3;

/** SWITCH *********************************************************/
//#define mInitAllSwitches()  TRISAbits.TRISA2=1;TRISAbits.TRISA3=1;
//#define mInitSwitch2()      TRISAbits.TRISA2=1;
//#define mInitSwitch3()      TRISAbits.TRISA3=1;
//#define sw2                 PORTAbits.RA2
//#define sw3                 PORTAbits.RA3

/** POT ************************************************************/
//#define mInitPOT()          {TRISAbits.TRISA0=1;ANSELAbits.ANSA0=1;ADCON1=0;ADCON2=0xBE;ADCON0=0x01;}


/** I/O pin definitions ********************************************/
#define INPUT_PIN 1
#define OUTPUT_PIN 0

/** MFRC522 control pin  *******************************************/
#define mRC522_Reset()          (LATBbits.LATB2)
#define mRC522_ChipSelect()     (LATBbits.LATB4)
//#define mRC522_ChipSelect()     (LATCbits.LATC7)

/** LCD interface  *************************************************/
//#define LCD_DATA4               (LATBbits.LATB4)
//#define LCD_DATA5               (LATBbits.LATB5)
//#define LCD_DATA6               (LATBbits.LATB6)
//#define LCD_DATA7               (LATBbits.LATB7)
//#define LCD_EN                  (LATAbits.LATA6)
//#define LCD_RS                  (LATCbits.LATC0)
//#define LCD_DATA_PORT           (LATB)

#endif  //HARDWARE_PROFILE_PICDEM_FSUSB_K50_H
