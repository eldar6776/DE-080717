/**
/**
/**
 ******************************************************************************
 * File Name          : mfrc522.c
 * Date               : 28/02/2016 23:16:19
 * Description        : mifare RC522 software modul
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <spi.h>
#include <delays.h>
#include "rc522.h"
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"

/* Private defines    --------------------------------------------------------*/
//#define CARD_TEST		1
//#define CARD_CLEAR		2

/* Private types  -----------------------------------------------------------*/
typedef enum
{
    MI_ERR = 0x00,
    MI_OK = 0x01,
    MI_NOTAGERR = 0x02

} RC522_StatusTypeDef;

typedef struct
{
    BYTE card_status;
    BYTE aUserCardID[5];
    BYTE card_user_group;
    BYTE card_usage_type;
    BYTE card_number_of_users;
    BYTE aCardExpiryTime[6];

} RC522_CardDataTypeDef;



/* Private variables  --------------------------------------------------------*/
BYTE mifare_rx_buffer[RC522_MAX_LEN];
BYTE aMifareAuthenticationKeyA[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; //buffer A password, 16 buffer, the password of every buffer is 6 bytes 
BYTE i, status;
BYTE aCardID[5] = {0, 0, 0, 0, 0}; //Recognized card ID
BYTE aCardSerial[5] = {0, 0, 0, 0, 0};
BYTE rc522_config;
BYTE aCardUserGroup[16];
BYTE aCardUsageType[16];
BYTE card_max_users_cnt;
BYTE rc522_sector_nr;
BYTE rc522_command;

RC522_CardDataTypeDef sCardDataMifare1;
RC522_SectorTypeDef Sector;


static enum
{
    RC522_UNSELECT = 0x00,
    RC522_READER_1 = 0x01,
    RC522_READER_2 = 0x02,
    RC522_READER_3 = 0x03

} RC522_Selected = RC522_UNSELECT;

/* Private macros   ----------------------------------------------------------*/

/* Private prototypes    -----------------------------------------------------*/
RC522_StatusTypeDef RC522_Check(BYTE* id);
RC522_StatusTypeDef RC522_Compare(BYTE* aCardID, BYTE* CompareID);
void RC522_WriteRegister(BYTE addr, BYTE val);
BYTE RC522_ReadRegister(BYTE addr);
void RC522_SetBitMask(BYTE reg, BYTE mask);
void RC522_ClearBitMask(BYTE reg, BYTE mask);
void RC522_AntennaOn(void);
void RC522_AntennaOff(void);
RC522_StatusTypeDef RC522_Reset(void);
RC522_StatusTypeDef RC522_Request(BYTE reqMode, BYTE* TagType);
RC522_StatusTypeDef RC522_ToCard(BYTE command, BYTE* sendData, BYTE sendLen, BYTE* backData, WORD* backLen);
RC522_StatusTypeDef RC522_Anticoll(BYTE* serNum);
void RC522_CalculateCRC(BYTE* pIndata, BYTE len, BYTE* pOutData);
BYTE RC522_SelectTag(BYTE* serNum);
RC522_StatusTypeDef RC522_Auth(BYTE authMode, BYTE BlockAddr, BYTE* Sectorkey, BYTE* serNum);
RC522_StatusTypeDef RC522_Read(BYTE blockAddr, BYTE* recvData);
RC522_StatusTypeDef RC522_Write(BYTE blockAddr, BYTE* writeData);
void RC522_Halt(void);
void RC522_ReadCard(BYTE sector);
void RC522_WriteCard(BYTE sector);
void RC522_ClearData(void);

/* Program code   ------------------------------------------------------------*/
void RC522_Init(void)
{
    rc522_command = RC522_IDLE;
    mRC522_ChipSelect() = 1;
    mRC522_Reset() = 0;
    Delay10KTCYx(1);
    mRC522_Reset() = 1;
    Delay10KTCYx(1);
    RC522_Reset();

}// End of mifare modul init

void RC522_Service(void)
{
    if (RC522_Check(aCardSerial) == MI_OK)
    {
        if (rc522_command == RC522_READ) RC522_ReadCard(rc522_sector_nr);
        else if (rc522_command == RC522_WRITE) RC522_WriteCard(rc522_sector_nr);
        //RC522_ClearData();
    }
    else
    {
        aCardSerial[0] = 0;
        aCardSerial[1] = 0;
        aCardSerial[2] = 0;
        aCardSerial[3] = 0;
        aCardSerial[4] = 0;
    }

    RC522_Reset();
    

}// End of mifare service function

RC522_StatusTypeDef RC522_Check(BYTE* id)
{

    RC522_StatusTypeDef status;

    status = RC522_Request(PICC_REQIDL, id); // Find cards, return card type

    if (status == MI_OK)
    {
        status = RC522_Anticoll(id); // Anti-collision, return card serial number 4 bytes
    }

    RC522_Halt();

    return (status);

}


RC522_StatusTypeDef RC522_Compare(BYTE* aCardID, BYTE* CompareID)
{
    BYTE i;

    for (i = 0; i < 5; i++)
    {
        if (aCardID[i] != CompareID[i])
        {
            return (MI_ERR);
        }
    }

    return (MI_OK);
}

void RC522_WriteRegister(BYTE addr, BYTE val)
{
    mRC522_ChipSelect() = 0;
    Delay10TCYx(1);
    addr = (addr << 1) & 0x7E;
    WriteSPI(addr);
    WriteSPI(val);
    mRC522_ChipSelect() = 1;
}// End of write register function

BYTE RC522_ReadRegister(BYTE addr)
{

    mRC522_ChipSelect() = 0; // set dummy value
    Delay10TCYx(1);
    addr = ((addr << 1) & 0x7E) | 0x80;
    WriteSPI(addr);
    mifare_rx_buffer[1] = ReadSPI();
    mRC522_ChipSelect() = 1;
    return (mifare_rx_buffer[1]);

}// End of read register function

void RC522_SetBitMask(BYTE reg, BYTE mask)
{
    RC522_WriteRegister(reg, RC522_ReadRegister(reg) | mask);

}// End of set bit mask function

void RC522_ClearBitMask(BYTE reg, BYTE mask)
{

    RC522_WriteRegister(reg, RC522_ReadRegister(reg) & (~mask));

}// End of clear bit mask function

void RC522_AntennaOn(void)
{

    BYTE temp;

    temp = RC522_ReadRegister(RC522_REG_TX_CONTROL);

    if (!(temp & 0x03))
    {

        RC522_SetBitMask(RC522_REG_TX_CONTROL, 0x03);

    }// End of if...

}// End of antena on function

void RC522_AntennaOff(void)
{
    RC522_ClearBitMask(RC522_REG_TX_CONTROL, 0x03);

}// End of antena off function

RC522_StatusTypeDef RC522_Reset(void)
{

    static DWORD delay = 0;
    /**
     *   Issue the SoftReset command.
     */
    RC522_WriteRegister(RC522_REG_COMMAND, PCD_RESETPHASE);
    /**
     *   The datasheet does not mention how long the SoftRest command takes to complete.
     *   But the RC522 might have been in soft power-down mode (triggered by bit 4 of CommandReg)
     *   Section 8.8.2 in the datasheet says the oscillator start-up time is the start up time of the crystal + 37,74us. Let us be generous: 50ms.
     */
    Delay10KTCYx(100);
    /**
     *   Wait for the PowerDown bit in CommandReg to be cleared
     */
    while (RC522_ReadRegister(RC522_REG_COMMAND) & (1 << 4))
    {
        /**
         *   RC522 still restarting - unlikely after waiting 50ms and more
         *   mifare modul is unresponsive so return error status
         */
        if (++delay == 0xffff)
        {

            return (MI_ERR);

        }// End of if...
    }// End of while...
    /**
     *   reset finished - return OK flag
     */
    RC522_WriteRegister(RC522_REG_T_MODE, 0x8D);
    RC522_WriteRegister(RC522_REG_T_PRESCALER, 0x3E);
    RC522_WriteRegister(RC522_REG_T_RELOAD_L, 30);
    RC522_WriteRegister(RC522_REG_T_RELOAD_H, 0);
    /* 48dB gain */
    //RC522_WriteRegister(RC522_REG_RF_CFG, 0x70);
    RC522_WriteRegister(RC522_REG_TX_AUTO, 0x40);
    RC522_WriteRegister(RC522_REG_MODE, 0x3D);
    RC522_AntennaOn(); // Open the antenna
    return (MI_OK);

}// End of software reset function

RC522_StatusTypeDef RC522_Request(BYTE reqMode, BYTE* TagType)
{

    RC522_StatusTypeDef status;
    WORD backBits; //The received data bits

    RC522_WriteRegister(RC522_REG_BIT_FRAMING, 0x07); // TxLastBits = BitFramingReg[2..0]	???

    TagType[0] = reqMode;
    status = RC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

    if ((status != MI_OK) || (backBits != 0x10))
    {

        status = MI_ERR;

    }// End of if...

    return (status);

}// End of request function

RC522_StatusTypeDef RC522_ToCard(BYTE command, BYTE* sendData, BYTE sendLen, BYTE* backData, WORD* backLen)
{

    RC522_StatusTypeDef status = MI_ERR;
    BYTE irqEn = 0x00;
    BYTE waitIRq = 0x00;
    BYTE lastBits;
    BYTE n;
    WORD i;


    switch (command)
    {

    case PCD_AUTHENT:
    {

        irqEn = 0x12;
        waitIRq = 0x10;

        break;

    }// End of case pcd authent

    case PCD_TRANSCEIVE:
    {

        irqEn = 0x77;
        waitIRq = 0x30;

        break;

    }// End of case pcd tranceive

    default:
    {

        break;

    }// End of default case


    }// End of switch command

    RC522_WriteRegister(RC522_REG_COMM_IE_N, irqEn | 0x80);
    RC522_ClearBitMask(RC522_REG_COMM_IRQ, 0x80);
    RC522_SetBitMask(RC522_REG_FIFO_LEVEL, 0x80);
    RC522_WriteRegister(RC522_REG_COMMAND, PCD_IDLE);

    //Writing data to the FIFO
    for (i = 0; i < sendLen; i++)
    {

        RC522_WriteRegister(RC522_REG_FIFO_DATA, sendData[i]);

    }// End of for loop

    //Execute the command
    RC522_WriteRegister(RC522_REG_COMMAND, command);

    if (command == PCD_TRANSCEIVE)
    {

        RC522_SetBitMask(RC522_REG_BIT_FRAMING, 0x80); //StartSend=1,transmission of data starts  

    }// End of if... 
    /**
     *   Waiting to receive data to complete
     */
    i = 2000; //i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms???
    do
    {
        /**
         *   CommIrqReg[7..0]
         *   Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
         */
        n = RC522_ReadRegister(RC522_REG_COMM_IRQ);
        i--;

    }
    while ((i != 0) && !(n & 0x01) && !(n & waitIRq)); // End of do...while loop              
    /**
     *   StartSend=0
     */
    RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);

    if (i != 0)
    {

        if (!(RC522_ReadRegister(RC522_REG_ERROR) & 0x1B))
        {

            status = MI_OK;

            if (n & irqEn & 0x01)
            {

                status = MI_NOTAGERR;

            }// End of if...

            if (command == PCD_TRANSCEIVE)
            {

                n = RC522_ReadRegister(RC522_REG_FIFO_LEVEL);
                lastBits = RC522_ReadRegister(RC522_REG_CONTROL) & 0x07;

                if (lastBits) *backLen = (n - 1)*8 + lastBits;
                else *backLen = n * 8;

                if (n == 0) n = 1;

                if (n > RC522_MAX_LEN) n = RC522_MAX_LEN;
                /**
                 *   Reading the received data in FIFO
                 */
                for (i = 0; i < n; i++)
                {

                    backData[i] = RC522_ReadRegister(RC522_REG_FIFO_DATA);

                }// End of for loop
            }// End of if (command == PCD_TRANSCEIVE)

        }
        else
        {

            status = MI_ERR;

        }// End of else
    }// End of if (i != 0)

    return (status);

}// End of to card function

RC522_StatusTypeDef RC522_Anticoll(BYTE* serNum)
{

    RC522_StatusTypeDef status;
    BYTE i;
    BYTE serNumCheck = 0;
    WORD unLen;

    RC522_WriteRegister(RC522_REG_BIT_FRAMING, 0x00); // TxLastBists = BitFramingReg[2..0]

    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = RC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
    {
        /**
         *   Check card serial number
         */
        for (i = 0; i < 4; i++)
        {

            serNumCheck ^= serNum[i];

        }// End of for loop....

        if (serNumCheck != serNum[i])
        {

            status = MI_ERR;

        }// End of if...
    }// End of if (status == MI_OK)

    return (status);

}// End of anticollision function

void RC522_CalculateCRC(BYTE* pIndata, BYTE len, BYTE* pOutData)
{

    BYTE i, n;

    RC522_ClearBitMask(RC522_REG_DIV_IRQ, 0x04); //CRCIrq = 0
    RC522_SetBitMask(RC522_REG_FIFO_LEVEL, 0x80); //Clear the FIFO pointer
    /**
     *   Write_RC522(CommandReg, PCD_IDLE);
     *   Writing data to the FIFO
     */
    for (i = 0; i < len; i++)
    {

        RC522_WriteRegister(RC522_REG_FIFO_DATA, *(pIndata + i));

    }// End of for loop...

    RC522_WriteRegister(RC522_REG_COMMAND, PCD_CALCCRC);

    //Wait CRC calculation is complete
    i = 0xFF;
    do
    {
        n = RC522_ReadRegister(RC522_REG_DIV_IRQ);
        i--;
    }
    while ((i != 0) && !(n & 0x04)); //CRCIrq = 1

    //Read CRC calculation result
    pOutData[0] = RC522_ReadRegister(RC522_REG_CRC_RESULT_L);
    pOutData[1] = RC522_ReadRegister(RC522_REG_CRC_RESULT_M);

}// End of calculate CRC function

BYTE RC522_SelectTag(BYTE* serNum)
{

    RC522_StatusTypeDef status;
    BYTE i;
    BYTE size;
    WORD recvBits;
    BYTE buffer[9];

    buffer[0] = PICC_SELECTTAG;
    buffer[1] = 0x70;

    for (i = 0; i < 5; i++)
    {

        buffer[i + 2] = *(serNum + i);

    }// End of for loop...

    RC522_CalculateCRC(buffer, 7, &buffer[7]); //??
    status = RC522_ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);

    if ((status == MI_OK) && (recvBits == 0x18))
    {

        size = buffer[0];

    }
    else
    {

        size = 0;

    }// End of else

    return (size);

}// End of select tag function

RC522_StatusTypeDef RC522_Auth(BYTE authMode, BYTE BlockAddr, BYTE* Sectorkey, BYTE* serNum)
{

    RC522_StatusTypeDef status;
    WORD recvBits;
    BYTE i;
    BYTE buff[12];

    //Verify the command block address + sector + password + card serial number
    buff[0] = authMode;
    buff[1] = BlockAddr;

    for (i = 0; i < 6; i++)
    {

        buff[i + 2] = *(Sectorkey + i);

    }// End of for loop...

    for (i = 0; i < 4; i++)
    {

        buff[i + 8] = *(serNum + i);

    }// End of for loop...

    status = RC522_ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);

    if ((status != MI_OK) || (!(RC522_ReadRegister(RC522_REG_STATUS2) & 0x08)))
    {

        status = MI_ERR;

    }// End of if....

    return (status);

}// End of auth function

RC522_StatusTypeDef RC522_Read(BYTE blockAddr, BYTE* recvData)
{

    RC522_StatusTypeDef status;
    WORD unLen;

    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    RC522_CalculateCRC(recvData, 2, &recvData[2]);
    status = RC522_ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

    if ((status != MI_OK) || (unLen != 0x90))
    {

        status = MI_ERR;

    }// End of if...

    return (status);

}// End of read function

RC522_StatusTypeDef RC522_Write(BYTE blockAddr, BYTE* writeData)
{

    RC522_StatusTypeDef status;
    WORD recvBits;
    BYTE i;
    BYTE buff[18];

    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    RC522_CalculateCRC(buff, 2, &buff[2]);
    status = RC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0f) != 0x0a))
    {

        status = MI_ERR;

    }// End of if...

    if (status == MI_OK)
    {
        /**
         *   Data to the FIFO write 16Byte
         */
        for (i = 0; i < 16; i++)
        {

            buff[i] = *(writeData + i);

        }// End of for loop..

        RC522_CalculateCRC(buff, 16, &buff[16]);
        status = RC522_ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

        if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0f) != 0x0a))
        {

            status = MI_ERR;

        }// End of if...
    }// End of if (status == MI_OK)

    return (status);

}// End of write function

void RC522_Halt(void)
{

    WORD unLen;
    BYTE buff[4];

    buff[0] = PICC_HALT;
    buff[1] = 0;
    RC522_CalculateCRC(buff, 2, &buff[2]);

    RC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen);

}// End of halt function

void RC522_ReadCard(BYTE sector)
{

    status = RC522_Request(PICC_REQIDL, mifare_rx_buffer);
    status = RC522_Anticoll(mifare_rx_buffer);

    for (i = 0; i < 5; i++)
    {
        aCardID[i] = mifare_rx_buffer[i];
    }

    RC522_ClearData();
    status = RC522_SelectTag(aCardID);
    status = RC522_Auth(PICC_AUTHENT1A, sector, aMifareAuthenticationKeyA, aCardID);

    if (status == MI_OK)
    {
        RC522_Read(sector, Sector.B_Dat_0);
        RC522_Read(sector + 1, Sector.B_Dat_1);
        RC522_Read(sector + 2, Sector.B_Dat_2);

    }// End of if...

    RC522_Halt();

}// End of read card function

void RC522_WriteCard(BYTE sector)
{

    status = RC522_Request(PICC_REQIDL, mifare_rx_buffer);
    status = RC522_Anticoll(mifare_rx_buffer);

    for (i = 0; i < 5; i++)
    {
        aCardID[i] = mifare_rx_buffer[i];
    }

    status = RC522_SelectTag(aCardID);
    status = RC522_Auth(PICC_AUTHENT1A, sector, aMifareAuthenticationKeyA, aCardID);

    if (status == MI_OK)
    {
        if (sector != SECTOR_0) RC522_Write(sector, Sector.B_Dat_0);
        RC522_Write(sector + 1, Sector.B_Dat_1);
        RC522_Write(sector + 2, Sector.B_Dat_2);
        //if (rc522_command == RC522_WRITE_KEY) status = RC522_Write(Sector.BlockAddress.B_Add_3, Sector.BlockData.B_Dat_3);
    }// End of if...

    RC522_Halt();

}// End of write card function

void RC522_ClearData(void)
{
    BYTE i;

    for (i = 0; i < 16; i++)
    {
        Sector.B_Dat_0[i] = 0x7f;
        Sector.B_Dat_1[i] = 0x7f;
        Sector.B_Dat_2[i] = 0x7f;
    }
    rc522_command = 0xff;
    rc522_sector_nr = 0xff;
}
