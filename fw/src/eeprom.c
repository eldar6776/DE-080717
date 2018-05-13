/******************************************************************************
 *
 *                  Kontroler LED rasvjete
 *
 *                  EEPROM functions
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
#include "eeprom.h"
//
//
/**  E X P O R T E D   R A M   V A R I J A B L E ******************************/
//
//
/**  P R I V A T E   D E F I N E S    *****************************************/
//
//
/**  P R I V A T E   E N U M E R A T O R S   **********************************/
//
//
/**  P R I V A T E   V A R I J A B L E ****************************************/
//
//
/**  P R I V A T E   F L A G S   **********************************************/
//
//
/**  P R I V A T E   M A C R O S   ********************************************/
//
//
/**  P R I V A T E   F U N C T I O N   P R O T O T Y P E S   ******************/
//
//
//*****************************************************************************
//*****************************************************************************
//
//*******************     P R O G R A M  C O D E     *************************
//
//*****************************************************************************
//*****************************************************************************
//
//

// <editor-fold defaultstate="collapsed" desc="internal eeprom read byte">

unsigned char EEPROM_ReadByte (unsigned char address) {
    EECON1 = 0;
    EEADR = address;
    EECON1bits.RD = 1;
    return (EEDATA);
}// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="internal eeprom read int">

unsigned int EEPROM_ReadInt (unsigned char address) {
    volatile unsigned int data;
    data = EEPROM_ReadByte (address);
    data = (data << 8);
    data += EEPROM_ReadByte (address + 0x01);
    return data;
}// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="internal eeprom write byte">

void EEPROM_WriteByte (unsigned char address, unsigned char data) {
    
    EECON1 = 0; //ensure CFGS=0 and EEPGD=0
    EECON1bits.WREN = 1; //enable write to EEPROM
    EEADR = address; //setup Address
    EEDATA = data; //and data
    EECON2 = 0x55; //required sequence #1
    EECON2 = 0xaa; //#2
    EECON1bits.WR = 1; //#3 = actual write
    while (!PIR2bits.EEIF); //wait until finished
    EECON1bits.WREN = 0; //disable write to EEPROM
    while (EECON1bits.WR) continue;
}// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="internal eeprom write int">

void EEPROM_WriteInt (unsigned char address, unsigned int data) {
    EEPROM_WriteByte (address, (data >> 8));
    EEPROM_WriteByte ((address + 0x01), data);
}// </editor-fold>
