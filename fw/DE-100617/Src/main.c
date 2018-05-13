/********************************************************************
 FileName:     main.c
 Dependencies: See INCLUDES section
 Processor:		PIC18, PIC24, and PIC32 USB Microcontrollers
 Hardware:		This demo is natively intended to be used on Microchip USB demo
                                boards supported by the MCHPFSUSB stack.  See release notes for
                                support matrix.  This demo can be modified for use on other hardware
                                platforms.
 Complier:  	Microchip C18 (for PIC18), C30 (for PIC24), C32 (for PIC32)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the Ă˘â‚¬Ĺ›CompanyĂ˘â‚¬ĹĄ) for its PICĂ‚Â® Microcontroller is intended and
 supplied to you, the CompanyĂ˘â‚¬â„˘s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN Ă˘â‚¬Ĺ›AS ISĂ˘â‚¬ĹĄ CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

 ********************************************************************
 File Description: PIC18f25K50 rfid mifare programmer V2

 Change History:
  Rev   Description
  ----  -----------------------------------------
  1.0   Initial release
  2.1   Updated for simplicity and to use common
                     coding style
  2.7b  Improvements to USBCBSendResume(), to make it easier to use.
 ********************************************************************/

#ifndef MAIN_C
#define MAIN_C

/** INCLUDES *******************************************************/
#include <spi.h>
#include <delays.h>
#include "USB/usb.h"
#include "HardwareProfile.h"
#include "USB/usb_function_hid.h"
#include "rc522.h"
#include "eeprom.h"


/** CONFIGURATION **************************************************/
#pragma config PLLSEL   = PLL3X     // 3X PLL multiplier selected
#pragma config CFGPLLEN = OFF       // PLL turned on during execution
#pragma config CPUDIV   = NOCLKDIV  // 1:1 mode (for 48MHz CPU)
#pragma config LS48MHZ  = SYS48X8   // Clock div / 8 in Low Speed USB mode
#pragma config FOSC     = INTOSCIO  // HFINTOSC selected at powerup, no clock out
#pragma config PCLKEN   = OFF       // Primary oscillator driver
#pragma config FCMEN    = OFF       // Fail safe clock monitor
#pragma config IESO     = OFF       // Internal/external switchover (two speed startup)
#pragma config nPWRTEN  = OFF       // Power up timer
#pragma config BOREN    = SBORDIS   // BOR enabled
#pragma config nLPBOR   = ON        // Low Power BOR
#pragma config WDTEN    = SWON      // Watchdog Timer controlled by SWDTEN
#pragma config WDTPS    = 32768     // WDT postscalar
#pragma config PBADEN   = OFF       // Port B Digital/Analog Powerup Behavior
#pragma config SDOMX    = RB3       // SDO function location
#pragma config LVP      = OFF       // Low voltage programming
#pragma config MCLRE    = ON        // MCLR function enabled (RE3 disabled)
#pragma config STVREN   = ON        // Stack overflow reset
//#pragma config ICPRT  = OFF       // Dedicated ICPORT program/debug pins enable
#pragma config XINST    = OFF       // Extended instruction set

#define READ_ERROR      1
#define READ_OK         2
/** VARIABLES ******************************************************/
extern BYTE aCardSerial[5];
extern BYTE rc522_sector_nr;
extern BYTE rc522_command;
//extern typedef struct Sector;

#pragma udata USB_VARIABLES=0x500
unsigned char ReceivedDataBuffer[64];
unsigned char ToSendDataBuffer[64];

#pragma udata
USB_HANDLE USBOutHandle = 0; //USB handle.  Must be initialized to 0 at startup.
USB_HANDLE USBInHandle = 0; //USB handle.  Must be initialized to 0 at startup.
BOOL blinkStatusValid = TRUE;
WORD address = 0;
BYTE led_gn_cnt = 0;
BYTE led_rd_cnt = 0;
BYTE buzz_cnt = 0;
BYTE cmd_cnt = 0;
static BYTE cmd_state = 0;
static WORD timer = 0;

/** PRIVATE PROTOTYPES *********************************************/
void BlinkUSBStatus(void);
BOOL Switch2IsPressed(void);
BOOL Switch3IsPressed(void);
static void InitializeSystem(void);
void ProcessIO(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void USBCBSendResume(void);
void CardDataFail(void);
BYTE ReadHotelCard(void);
void CopyCardData(void);
void Restart(void);
void CardDataSucceed(void);

/** VECTOR REMAPPING ***********************************************/

//On PIC18 devices, addresses 0x00, 0x08, and 0x18 are used for
//the reset, high priority interrupt, and low priority interrupt
//vectors.  However, the Microchip HID bootloader occupies the
//0x00-0xFFF program memory region.  Therefore, the bootloader code remaps 
//these vectors to new locations as indicated below.  This remapping is 
//only necessary if you wish to be able to (optionally) program the hex file 
//generated from this project with the USB bootloader.  
#define REMAPPED_RESET_VECTOR_ADDRESS           0x1000
#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1008
#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1018
#define APP_VERSION_ADDRESS                     0x1016 //Fixed location, so the App FW image version can be read by the bootloader.
#define APP_SIGNATURE_ADDRESS                   0x1006 //Signature location that must be kept at blaknk value (0xFFFF) in this project (has special purpose for bootloader).

//--------------------------------------------------------------------------
//Application firmware image version values, as reported to the bootloader
//firmware.  These are useful so the bootloader can potentially know if the
//user is trying to program an older firmware image onto a device that
//has already been programmed with a with a newer firmware image.
//Format is APP_FIRMWARE_VERSION_MAJOR.APP_FIRMWARE_VERSION_MINOR.
//The valid minor version is from 00 to 99.  Example:
//if APP_FIRMWARE_VERSION_MAJOR == 1, APP_FIRMWARE_VERSION_MINOR == 1,
//then the version is "1.01"
#define APP_FIRMWARE_VERSION_MAJOR  1   //valid values 0-255
#define APP_FIRMWARE_VERSION_MINOR  0   //valid values 0-99
//--------------------------------------------------------------------------

#pragma romdata AppVersionAndSignatureSection = APP_VERSION_ADDRESS
ROM unsigned char AppVersion[2] = {APP_FIRMWARE_VERSION_MINOR, APP_FIRMWARE_VERSION_MAJOR};
#pragma romdata AppSignatureSection = APP_SIGNATURE_ADDRESS
ROM unsigned short int SignaturePlaceholder = 0xFFFF;

#pragma code HIGH_INTERRUPT_VECTOR = 0x08

void High_ISR(void) {
    _asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
}
#pragma code LOW_INTERRUPT_VECTOR = 0x18

void Low_ISR(void) {
    _asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
}
extern void _startup(void); // See c018i.c in your C18 compiler dir
#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS

void _reset(void) {
    _asm goto _startup _endasm
}
#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS

void Remapped_High_ISR(void) {
    _asm goto YourHighPriorityISRCode _endasm
}
#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS

void Remapped_Low_ISR(void) {
    _asm goto YourLowPriorityISRCode _endasm
}
#pragma code


//These are your actual interrupt handling routines.
#pragma interrupt YourHighPriorityISRCode

void YourHighPriorityISRCode() {
    //Check which interrupt flag caused the interrupt.
    //Service the interrupt
    //Clear the interrupt flag
    //Etc.
#if defined(USB_INTERRUPT)
    USBDeviceTasks();
#endif

} //This return will be a "retfie fast", since this is in a #pragma interrupt section 
#pragma interruptlow YourLowPriorityISRCode

void YourLowPriorityISRCode() {
    //Check which interrupt flag caused the interrupt.
    //Service the interrupt
    //Clear the interrupt flag
    //Etc.

} //This return will be a "retfie", since this is in a #pragma interruptlow section 


/** DECLARATIONS ***************************************************/
#if defined(__18CXX)
#pragma code
#endif


/********************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *******************************************************************/
#if defined(__18CXX)
void main(void)
#else

int main(void)
#endif
{
    InitializeSystem();
    RC522_Init();
#if defined(USB_INTERRUPT)
    USBDeviceAttach();
#endif

    while (1) {
#if defined(USB_POLLING)
        // Check bus status and service USB interrupts.
        USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
        // this function periodically.  This function will take care
        // of processing and responding to SETUP transactions 
        // (such as during the enumeration process when you first
        // plug in).  USB hosts require that USB devices should accept
        // and process SETUP packets in a timely fashion.  Therefore,
        // when using polling, this function should be called 
        // regularly (such as once every 1.8ms or faster** [see 
        // inline code comments in usb_device.c for explanation when
        // "or faster" applies])  In most cases, the USBDeviceTasks() 
        // function does not take very long to execute (ex: <100 
        // instruction cycles) before it returns.
#endif


        // Application-specific tasks.
        // Application related code may be added here, or in the ProcessIO() function.
        ProcessIO();
    }//end while
}//end main

/********************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.                  
 *
 * Note:            None
 *******************************************************************/
static void InitializeSystem(void) {
    OSCTUNE = 0x80; //3X PLL ratio mode selected
    OSCCON = 0x70; //Switch to 16MHz HFINTOSC
    OSCCON2 = 0x10; //Enable PLL, SOSC, PRI OSC drivers turned off
    while (OSCCON2bits.PLLRDY != 1); //Wait for PLL lock
    *((unsigned char*) 0xFB5) = 0x90; //Enable active clock tuning for USB operation)
    ANSELA = 0x01;
    ANSELB = 0x00;
    ANSELC = 0x00;
    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    TRISA = 0x6f;
    TRISB = 0xe1;
    TRISC = 0xfd;
//    TRISA = 0x00;
//    TRISB = 0x00;
//    TRISC = 0x00;
    USBOutHandle = 0;
    USBInHandle = 0;
    BlueLed_Off();
    RedLed_On();
    Buzzer_Off();
    blinkStatusValid = TRUE;
    if (EEPROM_ReadByte(0x00) == 'U') EEPROM_WriteByte(0x00, 0x00);
    OpenSPI(SPI_FOSC_64, MODE_00, SMPEND);
    USBDeviceInit();

}//end InitializeSystem

/********************************************************************
 * Function:        void ProcessIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user
 *                  routines. It is a mixture of both USB and
 *                  non-USB tasks.
 *
 * Note:            None
 *******************************************************************/
void ProcessIO(void) {

    //Blink the LEDs according to the USB device status
    if (blinkStatusValid) BlinkUSBStatus();

    // User Application USB tasks
    if ((USBDeviceState < CONFIGURED_STATE) || (USBSuspendControl == 1)) return;

    //Check if we have received an OUT data packet from the host
    if (!HIDRxHandleBusy(USBOutHandle)) {
        //We just received a packet of data from the USB host.
        //Check the first byte of the packet to see what command the host 
        //application software wants us to fulfill.
        switch (ReceivedDataBuffer[0]) //Look at the data the host sent, to see what kind of application specific command it sent.
        {
                // <editor-fold defaultstate="collapsed" desc="read hotel card">
            case ('R'): //Read command      

                aCardSerial[0] = 0;
                aCardSerial[1] = 0;
                aCardSerial[2] = 0;
                aCardSerial[3] = 0;
                aCardSerial[4] = 0;
                BlueLed_Off();
                cmd_cnt = 0;
                led_gn_cnt = 0;
                led_rd_cnt = 0;
                buzz_cnt = 0;
                /*
                 *  --------------------------------  R E A D    S E C T O R   0
                 */
                rc522_command = RC522_READ;
                rc522_sector_nr = SECTOR_0;

                while (aCardSerial[0] == 0) {
                    RC522_Service();

                    if (++led_gn_cnt == 10) {
                        RedLed_Toggle();
                        led_gn_cnt = 0;

                        if (++cmd_cnt == 10) {
                            CardDataFail();
                            break;
                        }
                    }
                }

                if (ReadHotelCard() != READ_OK) break;

                if (!HIDTxHandleBusy(USBInHandle)) {
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*) & ToSendDataBuffer[0], 64);
                }

                CardDataSucceed();
                break; // </editor-fold>

                // <editor-fold defaultstate="collapsed" desc="read user card">
            case ('C'):

                aCardSerial[0] = 0;
                aCardSerial[1] = 0;
                aCardSerial[2] = 0;
                aCardSerial[3] = 0;
                aCardSerial[4] = 0;
                BlueLed_Off();
                cmd_cnt = 0;
                led_gn_cnt = 0;
                led_rd_cnt = 0;
                buzz_cnt = 0;
                /*
                 *  --------------------------------  R E A D    S E C T O R   3
                 */
                rc522_command = RC522_READ;
                rc522_sector_nr = SECTOR_3;

                while (aCardSerial[0] == 0) {
                    RC522_Service();

                    if (++led_gn_cnt == 10) {
                        RedLed_Toggle();
                        led_gn_cnt = 0;

                        if (++cmd_cnt == 10) {
                            CardDataFail();
                            break;
                        }
                    }
                }

                CopyCardData();

                if (!HIDTxHandleBusy(USBInHandle)) {
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*) & ToSendDataBuffer[0], 64);
                }

                CardDataSucceed();
                break; // </editor-fold>

                // <editor-fold defaultstate="collapsed" desc="read card continously">
            case ('K'):

                cmd_state = 1; // read continously
                break; // </editor-fold>

                // <editor-fold defaultstate="collapsed" desc="stop continous reading">
            case ('S'):

                if (cmd_state == 1) cmd_state = 0;
                break; // </editor-fold>

                // <editor-fold defaultstate="collapsed" desc="write hotel card">
            case ('W'):

                aCardSerial[0] = 0;
                aCardSerial[1] = 0;
                aCardSerial[2] = 0;
                aCardSerial[3] = 0;
                aCardSerial[4] = 0;
                BlueLed_Off();

                for (cmd_cnt = 0; cmd_cnt < 64; cmd_cnt++) {
                    ToSendDataBuffer[cmd_cnt] = 0xff;
                }

                RC522_ClearData();
                rc522_sector_nr = SECTOR_0;
                rc522_command = RC522_READ;
                cmd_cnt = 0;
                led_gn_cnt = 0;
                led_rd_cnt = 0;
                buzz_cnt = 0;

                while (aCardSerial[0] == 0) {
                    RC522_Service();

                    if (++led_gn_cnt == 10) {
                        RedLed_Toggle();
                        led_gn_cnt = 0;
                        ++cmd_cnt;
                    } else if (cmd_cnt == 10) {
                        CardDataFail();
                        break;
                    }
                }

                if (cmd_cnt == 10) break;
                rc522_command = RC522_IDLE;

                if (!HIDTxHandleBusy(USBInHandle)) {
                    ToSendDataBuffer[0] = 'I';
                    ToSendDataBuffer[1] = aCardSerial[0];
                    ToSendDataBuffer[2] = aCardSerial[1];
                    ToSendDataBuffer[3] = aCardSerial[2];
                    ToSendDataBuffer[4] = aCardSerial[3];
                    ToSendDataBuffer[5] = aCardSerial[4];
                    /*
                     *  SECTOR 0
                     */
                    RC522_ClearData();

                    for (cmd_cnt = 0; cmd_cnt < 16; cmd_cnt++) {
                        Sector.B_Dat_1[cmd_cnt] = ReceivedDataBuffer[cmd_cnt + 31];
                        Sector.B_Dat_2[cmd_cnt] = ReceivedDataBuffer[cmd_cnt + 47];
                    }

                    rc522_sector_nr = SECTOR_0;
                    rc522_command = RC522_WRITE;
                    RC522_Service();
                    RC522_ClearData();
                    /*
                     *  SECTOR 1
                     */
                    if ((ReceivedDataBuffer[1] == CARD_USER_GROUP_GUEST) || \
                    (ReceivedDataBuffer[1] == CARD_USER_GROUP_HANDMAID) || \
                    (ReceivedDataBuffer[1] == CARD_USER_GROUP_MANAGER) || \
                    (ReceivedDataBuffer[1] == CARD_USER_GROUP_SERVICE) || \
                    (ReceivedDataBuffer[1] == CARD_USER_GROUP_PRESET)) {
                        Sector.B_Dat_0[1] = ReceivedDataBuffer[1];
                    } else {
                        CardDataFail();
                        break;
                    }

                    for (cmd_cnt = 0; cmd_cnt < 5; cmd_cnt++) {
                        Sector.B_Dat_1[cmd_cnt] = ReceivedDataBuffer[cmd_cnt + 21];
                    }

                    rc522_sector_nr = SECTOR_1;
                    rc522_command = RC522_WRITE;
                    RC522_Service();
                    RC522_ClearData();
                    /*
                     *  SECTOR 2
                     */
                    Sector.B_Dat_0[0] = ReceivedDataBuffer[2] << 4;
                    Sector.B_Dat_0[0] += ReceivedDataBuffer[3] & 0x0f;
                    Sector.B_Dat_0[1] = ReceivedDataBuffer[4] << 4;
                    Sector.B_Dat_0[1] += ReceivedDataBuffer[5] & 0x0f;
                    Sector.B_Dat_0[2] = ReceivedDataBuffer[6] << 4;
                    Sector.B_Dat_0[2] += ReceivedDataBuffer[7] & 0x0f;
                    Sector.B_Dat_0[3] = ReceivedDataBuffer[8] << 4;
                    Sector.B_Dat_0[3] += ReceivedDataBuffer[9] & 0x0f;
                    Sector.B_Dat_0[4] = ReceivedDataBuffer[10] << 4;
                    Sector.B_Dat_0[4] += ReceivedDataBuffer[11] & 0x0f;
                    Sector.B_Dat_0[5] = 0x00;

                    address = (ReceivedDataBuffer[12] - 0x30) * 10000;
                    address += (ReceivedDataBuffer[13] - 0x30) * 1000;

                    if (ReceivedDataBuffer[14] > 0x30) {
                        ReceivedDataBuffer[14] = ReceivedDataBuffer[14] - 0x30;

                        while (ReceivedDataBuffer[14]) {
                            address += 100;
                            --ReceivedDataBuffer[14];
                        }
                    }

                    address += (ReceivedDataBuffer[15] - 0x30) * 10;
                    address += (ReceivedDataBuffer[16] - 0x30);

                    Sector.B_Dat_0[6] = (address >> 8);
                    Sector.B_Dat_0[7] = (address & 0xff);

                    for (cmd_cnt = 8; cmd_cnt < 12; cmd_cnt++) {
                        Sector.B_Dat_0[cmd_cnt] = ReceivedDataBuffer[cmd_cnt + 9];
                    }

                    rc522_sector_nr = SECTOR_2;
                    rc522_command = RC522_WRITE;
                    RC522_Service();
                    RC522_ClearData();

                    /***********************************************************
                     * 
                     *          C H E C K    W R I T T E N     D A T A
                     * 
                     **********************************************************/
                    RC522_ClearData();
                    rc522_sector_nr = SECTOR_0;
                    rc522_command = RC522_READ;
                    aCardSerial[0] = 0;
                    cmd_cnt = 0;
                    led_gn_cnt = 0;
                    led_rd_cnt = 0;
                    buzz_cnt = 0;

                    while (aCardSerial[0] == 0) {
                        RC522_Service();

                        if (++led_gn_cnt == 10) {
                            RedLed_Toggle();
                            led_gn_cnt = 0;
                            ++cmd_cnt;
                        } else if (cmd_cnt == 10) {
                            CardDataFail();
                            break;
                        }
                    }

                    if (cmd_cnt == 10) break;

                    if ((ToSendDataBuffer[1] != aCardSerial[0]) ||  \
                        (ToSendDataBuffer[2] != aCardSerial[1]) ||  \
                        (ToSendDataBuffer[3] != aCardSerial[2]) ||  \
                        (ToSendDataBuffer[4] != aCardSerial[3]) ||  \
                        (ToSendDataBuffer[5] != aCardSerial[4])) {
                        CardDataFail();
                        break;
                    }
                    RC522_ClearData();
                    rc522_sector_nr = SECTOR_1;
                    rc522_command = RC522_READ;
                    aCardSerial[0] = 0;
                    cmd_cnt = 0;
                    led_gn_cnt = 0;
                    led_rd_cnt = 0;
                    buzz_cnt = 0;

                    while (aCardSerial[0] == 0) {
                        RC522_Service();

                        if (++led_gn_cnt == 10) {
                            RedLed_Toggle();
                            led_gn_cnt = 0;
                            ++cmd_cnt;
                        } else if (cmd_cnt == 10) {
                            CardDataFail();
                            break;
                        }
                    }

                    if (cmd_cnt == 10) break;


                    RC522_ClearData();
                    rc522_sector_nr = SECTOR_2;
                    rc522_command = RC522_READ;
                    aCardSerial[0] = 0;
                    cmd_cnt = 0;
                    led_gn_cnt = 0;
                    led_rd_cnt = 0;
                    buzz_cnt = 0;

                    while (aCardSerial[0] == 0) {
                        RC522_Service();

                        if (++led_gn_cnt == 10) {
                            RedLed_Toggle();
                            led_gn_cnt = 0;
                            ++cmd_cnt;
                        } else if (cmd_cnt == 10) {
                            CardDataFail();
                            break;
                        }
                    }

                    if (cmd_cnt == 10) break;

                    if (((Sector.B_Dat_0[0] & 0xf0) != (ReceivedDataBuffer[2] << 4)) ||  \
                        ((Sector.B_Dat_0[0] & 0x0f) != (ReceivedDataBuffer[3] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[1] & 0xf0) != (ReceivedDataBuffer[4] << 4)) ||  \
                        ((Sector.B_Dat_0[1] & 0x0f) != (ReceivedDataBuffer[5] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[2] & 0xf0) != (ReceivedDataBuffer[6] << 4)) ||  \
                        ((Sector.B_Dat_0[2] & 0x0f) != (ReceivedDataBuffer[7] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[3] & 0xf0) != (ReceivedDataBuffer[8] << 4)) ||  \
                        ((Sector.B_Dat_0[3] & 0x0f) != (ReceivedDataBuffer[9] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[4] & 0xf0) != (ReceivedDataBuffer[10] << 4)) ||  \
                        ((Sector.B_Dat_0[4] & 0x0f) != (ReceivedDataBuffer[11] & 0x0f)) ||  \
                        (Sector.B_Dat_0[5] != 0x00) ||  \
                        (Sector.B_Dat_0[6] != (address >> 8)) ||  \
                        (Sector.B_Dat_0[7] != (address & 0xff)) ||  \
                        (Sector.B_Dat_0[8] != ReceivedDataBuffer[17]) ||  \
                        (Sector.B_Dat_0[9] != ReceivedDataBuffer[18]) ||  \
                        (Sector.B_Dat_0[10] != ReceivedDataBuffer[19]) ||  \
                        (Sector.B_Dat_0[11] != ReceivedDataBuffer[20])) {
                        CardDataFail();
                        break;
                    }

                    if (!HIDTxHandleBusy(USBInHandle) && ToSendDataBuffer[0] != 0xff) {
                        USBInHandle = HIDTxPacket(HID_EP, (BYTE*) & ToSendDataBuffer[0], 64);
                    }

                    CardDataSucceed();
                }
                break; // </editor-fold>

                // <editor-fold defaultstate="collapsed" desc="write pool card">
            case ('P'):

                aCardSerial[0] = 0;
                aCardSerial[1] = 0;
                aCardSerial[2] = 0;
                aCardSerial[3] = 0;
                aCardSerial[4] = 0;
                BlueLed_Off();

                for (cmd_cnt = 0; cmd_cnt < 64; cmd_cnt++) {
                    ToSendDataBuffer[cmd_cnt] = 0xff;
                }
                /*
                 *   SECTOR 3 
                 */
                RC522_ClearData();
                rc522_sector_nr = SECTOR_3;
                rc522_command = RC522_READ;
                cmd_cnt = 0;
                led_gn_cnt = 0;
                led_rd_cnt = 0;
                buzz_cnt = 0;

                while (aCardSerial[0] == 0) {
                    RC522_Service();

                    if (++led_gn_cnt == 10) {
                        RedLed_Toggle();
                        led_gn_cnt = 0;

                        if (++cmd_cnt == 10) {
                            CardDataFail();
                            break;
                        }
                    }
                }

                rc522_command = RC522_IDLE;

                if (!HIDTxHandleBusy(USBInHandle)) {
                    ToSendDataBuffer[0] = 'I';
                    ToSendDataBuffer[1] = aCardSerial[0];
                    ToSendDataBuffer[2] = aCardSerial[1];
                    ToSendDataBuffer[3] = aCardSerial[2];
                    ToSendDataBuffer[4] = aCardSerial[3];
                    ToSendDataBuffer[5] = aCardSerial[4];

                    RC522_ClearData();
                    /***********************************************************
                     * 
                     *              SECTOR 3 BLOCK 0
                     * 
                     ***********************************************************
                     * 
                     *  expiry date & time
                     */
                    Sector.B_Dat_0[0] = ReceivedDataBuffer[1] << 4;
                    Sector.B_Dat_0[0] += ReceivedDataBuffer[2] & 0x0f;
                    Sector.B_Dat_0[1] = ReceivedDataBuffer[3] << 4;
                    Sector.B_Dat_0[1] += ReceivedDataBuffer[4] & 0x0f;
                    Sector.B_Dat_0[2] = ReceivedDataBuffer[5] << 4;
                    Sector.B_Dat_0[2] += ReceivedDataBuffer[6] & 0x0f;
                    Sector.B_Dat_0[3] = ReceivedDataBuffer[7] << 4;
                    Sector.B_Dat_0[3] += ReceivedDataBuffer[8] & 0x0f;
                    Sector.B_Dat_0[4] = ReceivedDataBuffer[9] << 4;
                    Sector.B_Dat_0[4] += ReceivedDataBuffer[10] & 0x0f;
                    Sector.B_Dat_0[5] = 0x00;
                    /**
                     *  number of users
                     */
                    Sector.B_Dat_0[6] = (ReceivedDataBuffer[11] << 4);
                    Sector.B_Dat_0[6] += ReceivedDataBuffer[12] & 0x0f;
                    /**
                     *  tag type
                     */
                    Sector.B_Dat_0[10] = ReceivedDataBuffer[13];
                    /**
                     *  card type
                     * 
                     */
                    Sector.B_Dat_0[7] = ReceivedDataBuffer[14];
                    /**
                     *  system id
                     */
                    address = (ReceivedDataBuffer[15] - 0x30) * 10000;
                    address += (ReceivedDataBuffer[16] - 0x30) * 1000;

                    if (ReceivedDataBuffer[17] > 0x30) {
                        ReceivedDataBuffer[17] = ReceivedDataBuffer[17] - 0x30;

                        while (ReceivedDataBuffer[17]) {
                            address += 100;
                            --ReceivedDataBuffer[17];
                        }
                    }

                    address += (ReceivedDataBuffer[18] - 0x30) * 10;
                    address += (ReceivedDataBuffer[19] - 0x30);

                    Sector.B_Dat_0[8] = (address >> 8);
                    Sector.B_Dat_0[9] = (address & 0xff);
                    /**
                     *  user group
                     */
                    Sector.B_Dat_0[11] = ReceivedDataBuffer[20];

                    if (ReceivedDataBuffer[20] == CARD_USER_GROUP_POOL) rc522_command = RC522_WRITE;
                    else {
                        CardDataFail();
                        break;
                    }

                    rc522_sector_nr = SECTOR_3;
                    RC522_Service();
                    /***********************************************************
                     * 
                     *          C H E C K    W R I T T E N     D A T A
                     * 
                     **********************************************************/
                    RC522_ClearData();
                    rc522_sector_nr = SECTOR_3;
                    rc522_command = RC522_READ;
                    aCardSerial[0] = 0;
                    cmd_cnt = 0;
                    led_gn_cnt = 0;
                    led_rd_cnt = 0;
                    buzz_cnt = 0;

                    while (aCardSerial[0] == 0) {
                        RC522_Service();

                        if (++led_gn_cnt == 10) {
                            RedLed_Toggle();
                            led_gn_cnt = 0;
                            ++cmd_cnt;
                        } else if (cmd_cnt == 10) {
                            CardDataFail();
                            break;
                        }
                    }

                    if (cmd_cnt == 10) break;

                    if ((ToSendDataBuffer[1] != aCardSerial[0]) ||  \
                        (ToSendDataBuffer[2] != aCardSerial[1]) ||  \
                        (ToSendDataBuffer[3] != aCardSerial[2]) ||  \
                        (ToSendDataBuffer[4] != aCardSerial[3]) ||  \
                        (ToSendDataBuffer[5] != aCardSerial[4])) {
                        CardDataFail();
                        break;
                    }

                    if (((Sector.B_Dat_0[0] & 0xf0) != (ReceivedDataBuffer[1] << 4)) ||  \
                        ((Sector.B_Dat_0[0] & 0x0f) != (ReceivedDataBuffer[2] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[1] & 0xf0) != (ReceivedDataBuffer[3] << 4)) ||  \
                        ((Sector.B_Dat_0[1] & 0x0f) != (ReceivedDataBuffer[4] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[2] & 0xf0) != (ReceivedDataBuffer[5] << 4)) ||  \
                        ((Sector.B_Dat_0[2] & 0x0f) != (ReceivedDataBuffer[6] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[3] & 0xf0) != (ReceivedDataBuffer[7] << 4)) ||  \
                        ((Sector.B_Dat_0[3] & 0x0f) != (ReceivedDataBuffer[8] & 0x0f)) ||  \
                        ((Sector.B_Dat_0[4] & 0xf0) != (ReceivedDataBuffer[9] << 4)) ||  \
                        ((Sector.B_Dat_0[4] & 0x0f) != (ReceivedDataBuffer[10] & 0x0f)) ||  \
                        (Sector.B_Dat_0[6] != ((ReceivedDataBuffer[11] << 4) + (ReceivedDataBuffer[12] & 0x0f))) ||  \
                        (Sector.B_Dat_0[7] != ReceivedDataBuffer[14]) ||  \
                        (Sector.B_Dat_0[8] != (address >> 8)) ||  \
                        (Sector.B_Dat_0[9] != (address & 0xff)) ||  \
                        (Sector.B_Dat_0[10] != ReceivedDataBuffer[13]) ||  \
                        (Sector.B_Dat_0[11] != ReceivedDataBuffer[20])) {
                        CardDataFail();
                        break;
                    }

                    if (!HIDTxHandleBusy(USBInHandle) && ToSendDataBuffer[0] != 0xff) {
                        USBInHandle = HIDTxPacket(HID_EP, (BYTE*) & ToSendDataBuffer[0], 64);
                    }

                    CardDataSucceed();
                }
                break; // </editor-fold>

                // <editor-fold defaultstate="collapsed" desc="start bootloader">
            case('U'):

                EEPROM_WriteByte(0x00, 'U');
                CardDataSucceed();
                Restart();
                break; // </editor-fold>
        }
        //Re-arm the OUT endpoint, so we can receive the next OUT data packet 
        //that the host may try to send us.
        USBOutHandle = HIDRxPacket(HID_EP, (BYTE*) & ReceivedDataBuffer, 64);
    }

    if (cmd_state == 0) {
        timer = 0xfff;
    } else if (cmd_state == 1) {
        if (++timer > 0xfff) {
            timer = 0;
            rc522_command = RC522_READ;
            rc522_sector_nr = SECTOR_0;
            RC522_Service();
            RedLed_Toggle();
        }

        if (aCardSerial[0] != 0) {
            if (ReadHotelCard() == READ_OK) {
                if (!HIDTxHandleBusy(USBInHandle)) {
                    USBInHandle = HIDTxPacket(HID_EP, (BYTE*) & ToSendDataBuffer[0], 64);
                }
            }
        }
    }
}//end ProcessIO

/********************************************************************
 * Function:        void BlinkUSBStatus(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        BlinkUSBStatus turns on and off LEDs 
 *                  corresponding to the USB device state.
 *
 * Note:            mLED macros can be found in HardwareProfile.h
 *                  USBDeviceState is declared and updated in
 *                  usb_device.c.
 *******************************************************************/
void BlinkUSBStatus(void) 
{
    static WORD led_count = 0;

    if (led_count == 0)led_count = 10000U;
    led_count--;

    if (USBSuspendControl == 1) 
    {
        BlueLed_Off();
        if (led_count == 0) RedLed_Toggle();
    }
    else 
    {
        if (USBDeviceState == DETACHED_STATE) 
        {
            RedLed_Off();
            BlueLed_Off();
        } 
        else if (USBDeviceState == ATTACHED_STATE) 
        {
            RedLed_Off();
            BlueLed_On();
        } 
        else if (USBDeviceState == POWERED_STATE)
        {
            RedLed_Off();
            BlueLed_On();
        } 
        else if (USBDeviceState == DEFAULT_STATE) 
        {
            RedLed_Off();
            BlueLed_Off();
        } 
        else if (USBDeviceState == ADDRESS_STATE) 
        {
            if (led_count == 0) RedLed_Toggle();
             BlueLed_Off();
        }
        else if (USBDeviceState == CONFIGURED_STATE) 
        {
            BlueLed_On();
        }
    }
}

// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************
// The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
// events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
// packets to your device.  In response to this, all USB devices are supposed to decrease their power
// consumption from the USB Vbus to <2.5mA* each.  The USB module detects this condition (which according
// to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
// function.  You should modify these callback functions to take appropriate actions for each of these
// conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
// consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
// microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
// add code that undoes the power saving things done in the USBCBSuspend() function.

// The USBCBSendResume() function is special, in that the USB stack will not automatically call this
// function.  This function is meant to be called from the application firmware instead.  See the
// additional comments near the function.

// Note *: The "usb_20.pdf" specs indicate 500uA or 2.5mA, depending upon device classification. However,
// the USB-IF has officially issued an ECN (engineering change notice) changing this to 2.5mA for all 
// devices.  Make sure to re-download the latest specifications to get all of the newest ECNs.

/******************************************************************************
 * Function:        void USBCBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Call back that is invoked when a USB suspend is detected
 *
 * Note:            None
 *****************************************************************************/
void USBCBSuspend(void) {
    //Example power saving code.  Insert appropriate code here for the desired
    //application behavior.  If the microcontroller will be put to sleep, a
    //process similar to that shown below may be used:

    //ConfigureIOPinsForLowPower();
    //SaveStateOfAllInterruptEnableBits();
    //DisableAllInterruptEnableBits();
    //EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
    //Sleep();
    //RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
    //RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

    //IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here.  This bit is 
    //cleared inside the usb_device.c file.  Clearing USBActivityIF here will cause 
    //things to not work as intended.	


#if defined(__C30__) || defined __XC16__
    //This function requires that the _IPL level be something other than 0.
    //  We can set it here to something other than 
#ifndef DSPIC33E_USB_STARTER_KIT
    _IPL = 1;
    USBSleepOnSuspend();
#endif
#endif
}

/******************************************************************************
 * Function:        void USBCBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The host may put USB peripheral devices in low power
 *					suspend mode (by "sending" 3+ms of idle).  Once in suspend
 *					mode, the host may wake the device back up by sending non-
 *					idle state signalling.
 *					
 *					This call back is invoked when a wakeup from USB suspend 
 *					is detected.
 *
 * Note:            None
 *****************************************************************************/
void USBCBWakeFromSuspend(void) {
    // If clock switching or other power savings measures were taken when
    // executing the USBCBSuspend() function, now would be a good time to
    // switch back to normal full power run mode conditions.  The host allows
    // 10+ milliseconds of wakeup time, after which the device must be 
    // fully back to normal, and capable of receiving and processing USB
    // packets.  In order to do this, the USB module must receive proper
    // clocking (IE: 48MHz clock must be available to SIE for full speed USB
    // operation).  
    // Make sure the selected oscillator settings are consistent with USB 
    // operation before returning from this function.
}

/********************************************************************
 * Function:        void USBCB_SOF_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB host sends out a SOF packet to full-speed
 *                  devices every 1 ms. This interrupt may be useful
 *                  for isochronous pipes. End designers should
 *                  implement callback routine as necessary.
 *
 * Note:            None
 *******************************************************************/
void USBCB_SOF_Handler(void) {
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.
}

/*******************************************************************
 * Function:        void USBCBErrorHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this callback is mainly for
 *                  debugging during development. Check UEIR to see
 *                  which error causes the interrupt.
 *
 * Note:            None
 *******************************************************************/
void USBCBErrorHandler(void) {
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

    // Typically, user firmware does not need to do anything special
    // if a USB error occurs.  For example, if the host sends an OUT
    // packet to your device, but the packet gets corrupted (ex:
    // because of a bad connection, or the user unplugs the
    // USB cable during the transmission) this will typically set
    // one or more USB error interrupt flags.  Nothing specific
    // needs to be done however, since the SIE will automatically
    // send a "NAK" packet to the host.  In response to this, the
    // host will normally retry to send the packet again, and no
    // data loss occurs.  The system will typically recover
    // automatically, without the need for application firmware
    // intervention.

    // Nevertheless, this callback function is provided, such as
    // for debugging purposes.
}

/*******************************************************************
 * Function:        void USBCBCheckOtherReq(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        When SETUP packets arrive from the host, some
 * 					firmware must process the request and respond
 *					appropriately to fulfill the request.  Some of
 *					the SETUP packets will be for standard
 *					USB "chapter 9" (as in, fulfilling chapter 9 of
 *					the official USB specifications) requests, while
 *					others may be specific to the USB device class
 *					that is being implemented.  For example, a HID
 *					class device needs to be able to respond to
 *					"GET REPORT" type of requests.  This
 *					is not a standard USB chapter 9 request, and 
 *					therefore not handled by usb_device.c.  Instead
 *					this request should be handled by class specific 
 *					firmware, such as that contained in usb_function_hid.c.
 *
 * Note:            None
 *******************************************************************/
void USBCBCheckOtherReq(void) {
    USBCheckHIDRequest();
}//end

/*******************************************************************
 * Function:        void USBCBStdSetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USBCBStdSetDscHandler() callback function is
 *					called when a SETUP, bRequest: SET_DESCRIPTOR request
 *					arrives.  Typically SET_DESCRIPTOR requests are
 *					not used in most applications, and it is
 *					optional to support this type of request.
 *
 * Note:            None
 *******************************************************************/
void USBCBStdSetDscHandler(void) {
    // Must claim session ownership if supporting this request
}//end

/*******************************************************************
 * Function:        void USBCBInitEP(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the device becomes
 *                  initialized, which occurs after the host sends a
 * 					SET_CONFIGURATION (wValue not = 0) request.  This 
 *					callback function should initialize the endpoints 
 *					for the device's usage according to the current 
 *					configuration.
 *
 * Note:            None
 *******************************************************************/
void USBCBInitEP(void) {
    //enable the HID endpoint
    USBEnableEndpoint(HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
    //Re-arm the OUT endpoint for the next packet
    USBOutHandle = HIDRxPacket(HID_EP, (BYTE*) & ReceivedDataBuffer, 64);
}

/********************************************************************
 * Function:        void USBCBSendResume(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB specifications allow some types of USB
 * 					peripheral devices to wake up a host PC (such
 *					as if it is in a low power suspend to RAM state).
 *					This can be a very useful feature in some
 *					USB applications, such as an Infrared remote
 *					control	receiver.  If a user presses the "power"
 *					button on a remote control, it is nice that the
 *					IR receiver can detect this signalling, and then
 *					send a USB "command" to the PC to wake up.
 *					
 *					The USBCBSendResume() "callback" function is used
 *					to send this special USB signalling which wakes 
 *					up the PC.  This function may be called by
 *					application firmware to wake up the PC.  This
 *					function will only be able to wake up the host if
 *                  all of the below are true:
 *					
 *					1.  The USB driver used on the host PC supports
 *						the remote wakeup capability.
 *					2.  The USB configuration descriptor indicates
 *						the device is remote wakeup capable in the
 *						bmAttributes field.
 *					3.  The USB host PC is currently sleeping,
 *						and has previously sent your device a SET 
 *						FEATURE setup packet which "armed" the
 *						remote wakeup capability.   
 *
 *                  If the host has not armed the device to perform remote wakeup,
 *                  then this function will return without actually performing a
 *                  remote wakeup sequence.  This is the required behavior, 
 *                  as a USB device that has not been armed to perform remote 
 *                  wakeup must not drive remote wakeup signalling onto the bus;
 *                  doing so will cause USB compliance testing failure.
 *                  
 *					This callback should send a RESUME signal that
 *                  has the period of 1-15ms.
 *
 * Note:            This function does nothing and returns quickly, if the USB
 *                  bus and host are not in a suspended condition, or are 
 *                  otherwise not in a remote wakeup ready state.  Therefore, it
 *                  is safe to optionally call this function regularly, ex: 
 *                  anytime application stimulus occurs, as the function will
 *                  have no effect, until the bus really is in a state ready
 *                  to accept remote wakeup. 
 *
 *                  When this function executes, it may perform clock switching,
 *                  depending upon the application specific code in 
 *                  USBCBWakeFromSuspend().  This is needed, since the USB
 *                  bus will no longer be suspended by the time this function
 *                  returns.  Therefore, the USB module will need to be ready
 *                  to receive traffic from the host.
 *
 *                  The modifiable section in this routine may be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of ~3-15 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at least 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *                    and verify the actual signal on an oscilloscope.
 *******************************************************************/
void USBCBSendResume(void) {
    static WORD delay_count;

    //First verify that the host has armed us to perform remote wakeup.
    //It does this by sending a SET_FEATURE request to enable remote wakeup,
    //usually just before the host goes to standby mode (note: it will only
    //send this SET_FEATURE request if the configuration descriptor declares
    //the device as remote wakeup capable, AND, if the feature is enabled
    //on the host (ex: on Windows based hosts, in the device manager 
    //properties page for the USB device, power management tab, the 
    //"Allow this device to bring the computer out of standby." checkbox 
    //should be checked).
    if (USBGetRemoteWakeupStatus() == TRUE) {
        //Verify that the USB bus is in fact suspended, before we send
        //remote wakeup signalling.
        if (USBIsBusSuspended() == TRUE) {
            USBMaskInterrupts();

            //Clock switch to settings consistent with normal USB operation.
            USBCBWakeFromSuspend();
            USBSuspendControl = 0;
            USBBusIsSuspended = FALSE; //So we don't execute this code again, 
            //until a new suspend condition is detected.

            //Section 7.1.7.7 of the USB 2.0 specifications indicates a USB
            //device must continuously see 5ms+ of idle on the bus, before it sends
            //remote wakeup signalling.  One way to be certain that this parameter
            //gets met, is to add a 2ms+ blocking delay here (2ms plus at 
            //least 3ms from bus idle to USBIsBusSuspended() == TRUE, yeilds
            //5ms+ total delay since start of idle).
            delay_count = 3600U;
            do {
                delay_count--;
            }            while (delay_count);

            //Now drive the resume K-state signalling onto the USB bus.
            USBResumeControl = 1; // Start RESUME signaling
            delay_count = 1800U; // Set RESUME line for 1-13 ms
            do {
                delay_count--;
            }            while (delay_count);
            USBResumeControl = 0; //Finished driving resume signalling

            USBUnmaskInterrupts();
        }
    }
}

/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size) {
    switch (event) {
        case EVENT_TRANSFER:
            //Add application specific callback task or callback function here if desired.
            break;
        case EVENT_SOF:
            USBCB_SOF_Handler();
            break;
        case EVENT_SUSPEND:
            USBCBSuspend();
            break;
        case EVENT_RESUME:
            USBCBWakeFromSuspend();
            break;
        case EVENT_CONFIGURED:
            USBCBInitEP();
            break;
        case EVENT_SET_DESCRIPTOR:
            USBCBStdSetDscHandler();
            break;
        case EVENT_EP0_REQUEST:
            USBCBCheckOtherReq();
            break;
        case EVENT_BUS_ERROR:
            USBCBErrorHandler();
            break;
        case EVENT_TRANSFER_TERMINATED:
            //Add application specific callback task or callback function here if desired.
            //The EVENT_TRANSFER_TERMINATED event occurs when the host performs a CLEAR
            //FEATURE (endpoint halt) request on an application endpoint which was 
            //previously armed (UOWN was = 1).  Here would be a good place to:
            //1.  Determine which endpoint the transaction that just got terminated was 
            //      on, by checking the handle value in the *pdata.
            //2.  Re-arm the endpoint if desired (typically would be the case for OUT 
            //      endpoints).
            break;
        default:
            break;
    }
    return TRUE;
}

void CardDataFail(void) {
    BYTE cnt;

    Buzzer_On();
    BlueLed_On();
    Delay10KTCYx(250);
    Delay10KTCYx(250);
    Delay10KTCYx(250);
    Buzzer_Off();
    BlueLed_Off();

    for (cnt = 0; cnt < 64; cnt++) {
        ReceivedDataBuffer[cnt] = 0xff;
        ToSendDataBuffer[cnt] = 0xff;
    }

    rc522_command = RC522_IDLE;
    ToSendDataBuffer[0] = 'F';
    ToSendDataBuffer[1] = 'A';
    ToSendDataBuffer[2] = 'I';
    ToSendDataBuffer[3] = 'L';

    if (!HIDTxHandleBusy(USBInHandle)) {
        USBInHandle = HIDTxPacket(HID_EP, (BYTE*) & ToSendDataBuffer[0], 64);
    }
}

void CardDataSucceed(void) {
    cmd_cnt = 0;

    while (cmd_cnt < 3) {
        Buzzer_On();
        BlueLed_On();
        Delay10KTCYx(40);
        Buzzer_Off();
        BlueLed_Off();
        Delay10KTCYx(100);
        cmd_cnt++;
    }

    cmd_cnt = 0;
}

BYTE ReadHotelCard(void) {
    ToSendDataBuffer[0] = 'I';
    ToSendDataBuffer[1] = aCardSerial[0];
    ToSendDataBuffer[2] = aCardSerial[1];
    ToSendDataBuffer[3] = aCardSerial[2];
    ToSendDataBuffer[4] = aCardSerial[3];
    ToSendDataBuffer[5] = aCardSerial[4];

    for (cmd_cnt = 0; cmd_cnt < 16; cmd_cnt++) {
        //ToSendDataBuffer[cmd_cnt + 32] = Sector.BlockData.B_Dat_1[cmd_cnt];
        ToSendDataBuffer[cmd_cnt + 48] = Sector.B_Dat_2[cmd_cnt];
    }

    /*
     *  --------------------------------  R E A D    S E C T O R   1
     */
    rc522_command = RC522_READ;
    rc522_sector_nr = SECTOR_1;

    cmd_cnt = 0;
    led_gn_cnt = 0;
    led_rd_cnt = 0;
    buzz_cnt = 0;
    aCardSerial[0] = 0;

    while (aCardSerial[0] == 0) {
        RC522_Service();

        if (++led_gn_cnt == 10) {
            RedLed_Toggle();
            led_gn_cnt = 0;

            if (++cmd_cnt == 10) {
                CardDataFail();
                return (READ_ERROR);
            }
        }
    }

    ToSendDataBuffer[6] = 'G';

    for (cmd_cnt = 0; cmd_cnt < 16; cmd_cnt++) {
        if ((Sector.B_Dat_0[cmd_cnt] == CARD_USER_GROUP_GUEST) || \
            (Sector.B_Dat_0[cmd_cnt] == CARD_USER_GROUP_HANDMAID) || \
            (Sector.B_Dat_0[cmd_cnt] == CARD_USER_GROUP_MANAGER) || \
            (Sector.B_Dat_0[cmd_cnt] == CARD_USER_GROUP_SERVICE) || \
            (Sector.B_Dat_0[cmd_cnt] == CARD_USER_GROUP_PRESET)) {
            ToSendDataBuffer[7] = Sector.B_Dat_0[cmd_cnt];
        }
    }

    ToSendDataBuffer[32] = Sector.B_Dat_1[0];
    ToSendDataBuffer[33] = Sector.B_Dat_1[1];
    ToSendDataBuffer[34] = Sector.B_Dat_1[2];
    ToSendDataBuffer[35] = Sector.B_Dat_1[3];
    ToSendDataBuffer[36] = Sector.B_Dat_1[4];
    /*
     *  --------------------------------  R E A D    S E C T O R   2
     */
    rc522_command = RC522_READ;
    rc522_sector_nr = SECTOR_2;

    cmd_cnt = 0;
    led_gn_cnt = 0;
    led_rd_cnt = 0;
    buzz_cnt = 0;
    aCardSerial[0] = 0;

    while (aCardSerial[0] == 0) {
        RC522_Service();

        if (++led_gn_cnt == 10) {
            RedLed_Toggle();
            led_gn_cnt = 0;

            if (++cmd_cnt == 10) {
                CardDataFail();
                return (READ_ERROR);
            }
        }
    }

    ToSendDataBuffer[8] = 'T';
    ToSendDataBuffer[9] = (Sector.B_Dat_0[0] >> 4) + 0x30;
    ToSendDataBuffer[10] = (Sector.B_Dat_0[0] & 0x0f) + 0x30;
    ToSendDataBuffer[11] = (Sector.B_Dat_0[1] >> 4) + 0x30;
    ToSendDataBuffer[12] = (Sector.B_Dat_0[1] & 0x0f) + 0x30;
    ToSendDataBuffer[13] = (Sector.B_Dat_0[2] >> 4) + 0x30;
    ToSendDataBuffer[14] = (Sector.B_Dat_0[2] & 0x0f) + 0x30;
    ToSendDataBuffer[15] = (Sector.B_Dat_0[3] >> 4) + 0x30;
    ToSendDataBuffer[16] = (Sector.B_Dat_0[3] & 0x0f) + 0x30;
    ToSendDataBuffer[17] = (Sector.B_Dat_0[4] >> 4) + 0x30;
    ToSendDataBuffer[18] = (Sector.B_Dat_0[4] & 0x0f) + 0x30;
    ToSendDataBuffer[19] = (Sector.B_Dat_0[5] >> 4) + 0x30;
    ToSendDataBuffer[20] = (Sector.B_Dat_0[5] & 0x0f) + 0x30;
    ToSendDataBuffer[21] = 'A';
    ToSendDataBuffer[22] = 0x30;
    ToSendDataBuffer[23] = 0x30;
    ToSendDataBuffer[24] = 0x30;
    ToSendDataBuffer[25] = 0x30;
    ToSendDataBuffer[26] = 0x30;

    address = Sector.B_Dat_0[6];
    address <<= 8;
    address += Sector.B_Dat_0[7];

    while (address > 9999) {
        address -= 10000;
        ++ToSendDataBuffer[22];
    }

    while (address > 999) {
        address -= 1000;
        ++ToSendDataBuffer[23];
    }

    while (address > 99) {
        address -= 100;
        ++ToSendDataBuffer[24];
    }


    while (address > 9) {
        address -= 10;
        ++ToSendDataBuffer[25];
    }

    ToSendDataBuffer[26] = address + 0x30;

    ToSendDataBuffer[27] = Sector.B_Dat_0[8];
    ToSendDataBuffer[28] = Sector.B_Dat_0[9];
    ToSendDataBuffer[29] = Sector.B_Dat_0[10];
    ToSendDataBuffer[30] = Sector.B_Dat_0[11];

    rc522_command = RC522_IDLE;
    return (READ_OK);
}

void CopyCardData(void) {
    // I = card serial
    // E = card expiry date&time
    // U = number of users
    // D = system id
    // C = tag type
    // G = user group
    // F = user image id
    // P = prepaid time
    // M = monday start - end time
    // T = tuesday start - end time
    // W = wednesday start - end time
    // H = thursday start - end time
    // F = friday start - end time
    // S = saturday start - end time
    // N = sunday start - end time
    //
    ToSendDataBuffer[0] = 'I';
    ToSendDataBuffer[1] = aCardSerial[0];
    ToSendDataBuffer[2] = aCardSerial[1];
    ToSendDataBuffer[3] = aCardSerial[2];
    ToSendDataBuffer[4] = aCardSerial[3];
    ToSendDataBuffer[5] = aCardSerial[4];
    ToSendDataBuffer[6] = 'E';
    ToSendDataBuffer[7] = Sector.B_Dat_0[0];
    ToSendDataBuffer[8] = Sector.B_Dat_0[1];
    ToSendDataBuffer[9] = Sector.B_Dat_0[2];
    ToSendDataBuffer[10] = Sector.B_Dat_0[3];
    ToSendDataBuffer[11] = Sector.B_Dat_0[4];
    ToSendDataBuffer[12] = 'U';
    ToSendDataBuffer[13] = Sector.B_Dat_0[5];
    ToSendDataBuffer[14] = 'D';
    ToSendDataBuffer[15] = Sector.B_Dat_0[7];
    ToSendDataBuffer[16] = Sector.B_Dat_0[8];
    ToSendDataBuffer[17] = 'C';
    ToSendDataBuffer[18] = Sector.B_Dat_0[9];
    ToSendDataBuffer[19] = 'G';
    ToSendDataBuffer[20] = Sector.B_Dat_0[10];
    ToSendDataBuffer[21] = 'F';
    ToSendDataBuffer[22] = Sector.B_Dat_0[11];
    ToSendDataBuffer[23] = Sector.B_Dat_0[12];
    ToSendDataBuffer[24] = 'P';
    ToSendDataBuffer[25] = Sector.B_Dat_0[13];
    ToSendDataBuffer[26] = 'M';
    ToSendDataBuffer[27] = Sector.B_Dat_1[0];
    ToSendDataBuffer[28] = Sector.B_Dat_1[1];
    ToSendDataBuffer[29] = Sector.B_Dat_1[2];
    ToSendDataBuffer[30] = Sector.B_Dat_1[3];
    ToSendDataBuffer[31] = 'T';
    ToSendDataBuffer[32] = Sector.B_Dat_1[4];
    ToSendDataBuffer[33] = Sector.B_Dat_1[5];
    ToSendDataBuffer[34] = Sector.B_Dat_1[6];
    ToSendDataBuffer[35] = Sector.B_Dat_1[7];
    ToSendDataBuffer[36] = 'W';
    ToSendDataBuffer[37] = Sector.B_Dat_1[8];
    ToSendDataBuffer[38] = Sector.B_Dat_1[9];
    ToSendDataBuffer[39] = Sector.B_Dat_1[10];
    ToSendDataBuffer[40] = Sector.B_Dat_1[11];
    ToSendDataBuffer[41] = 'H';
    ToSendDataBuffer[42] = Sector.B_Dat_1[12];
    ToSendDataBuffer[43] = Sector.B_Dat_1[13];
    ToSendDataBuffer[44] = Sector.B_Dat_1[14];
    ToSendDataBuffer[45] = Sector.B_Dat_1[15];
    ToSendDataBuffer[46] = 'F';
    ToSendDataBuffer[47] = Sector.B_Dat_2[0];
    ToSendDataBuffer[48] = Sector.B_Dat_2[1];
    ToSendDataBuffer[49] = Sector.B_Dat_2[2];
    ToSendDataBuffer[50] = Sector.B_Dat_2[3];
    ToSendDataBuffer[51] = 'S';
    ToSendDataBuffer[52] = Sector.B_Dat_2[4];
    ToSendDataBuffer[53] = Sector.B_Dat_2[5];
    ToSendDataBuffer[54] = Sector.B_Dat_2[6];
    ToSendDataBuffer[55] = Sector.B_Dat_2[7];
    ToSendDataBuffer[56] = 'N';
    ToSendDataBuffer[57] = Sector.B_Dat_2[8];
    ToSendDataBuffer[58] = Sector.B_Dat_2[9];
    ToSendDataBuffer[59] = Sector.B_Dat_2[10];
    ToSendDataBuffer[60] = Sector.B_Dat_2[11];
    ToSendDataBuffer[61] = 0x7f;
    ToSendDataBuffer[62] = 0x7f;
    ToSendDataBuffer[63] = 0x7f;
}

void Restart(void) {
    Reset();
}
/** EOF main.c *************************************************/
#endif

