/******************************************************************************
 *
 *                  Kontroler LED rasvjete
 *
 *                  EEPROM functions header
 *
 *******************************************************************************
 * Ime fajla:       eeprom.c
 *
 * Procesor:        PIC18F26K20
 *
 * Kompajler:       XC8(v1.32)
 *
 * Product Version: MPLAB X IDE v2.26
 *
 * Java:            1.7.0_67; Java HotSpot(TM) 64-Bit Server VM 24.65-b04
 *
 * System:          Windows 8.1 version 6.3 running on amd64; Cp1250;
 *
 * Userdir:         C:\Users\Eldar\AppData\Roaming\.mplab_ide\dev\v2.26
 *
 * Autor:           <mailto> eldar6776@hotmail.com
 *
 ******************************************************************************/
//
//
//
#ifndef __EEPROM_H
#define __EEPROM_H
//
//
/**  I N C L U D E S    *******************************************************/
//
//
#include <p18cxxx.h>
//
//
/**  T Y P E D E F S    *******************************************************/
//
//
/**  D E F I N E S    *********************************************************/
//


//
/** E X P O R T E D    V A R I J A B L E **************************************/
//
//
/** E N U M E R A T O R S   ***************************************************/
//
//
/** R A M   V A R I J A B L E *************************************************/
//
//
/** F L A G S   ***************************************************************/
//
//
/** M A C R O S   *************************************************************/
//
//
/** F U N C T I O N   P R O T O T Y P E S   ***********************************/
//
//
unsigned char EEPROM_ReadByte (unsigned char address);
unsigned int EEPROM_ReadInt (unsigned char address);
void EEPROM_WriteByte (unsigned char address, unsigned char data);
void EEPROM_WriteInt (unsigned char address, unsigned int data);
//
//
//
#endif	/* EEPROM_H */
