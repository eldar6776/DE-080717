/**
 ******************************************************************************
 * File Name          : rc522.h
 * Date               : 08/05/2016 23:15:16
 * Description        : mifare RC522 modul header
 ******************************************************************************
 *
 *
 ******************************************************************************
 */
#ifndef RC522_H
#define RC522_H                             200	// version 1.00

/* Includes ------------------------------------------------------------------*/

/* Exported defines    -------------------------------------------------------*/
#define RC522_PROCESS_TIME                  10

#define CARD_PENDING                        (0x00)
#define CARD_VALID                          (0x06)
#define CARD_INVALID                        (0x15)
#define CARD_DATA_FORMATED                  (0x7f)
/**
*--------------      card user groups predefine     ---------------------
*/
#define CARD_USER_GROUP_GUEST				('G')
#define CARD_USER_GROUP_HANDMAID			('H')
#define CARD_USER_GROUP_MANAGER				('M')
#define CARD_USER_GROUP_SERVICE				('S')
#define CARD_USER_GROUP_PRESET				('P')
#define CARD_USER_GROUP_KINDERGARDEN		('K')
#define CARD_USER_GROUP_POOL				('B')
#define CARD_USER_GROUP_PARKING				('R')
/**
*--------------      card type predefine     ---------------------
*/
#define CARD_TYPE_ONE_TIME					('O')
#define CARD_TYPE_MULTI						('E')
#define CARD_TYPE_PRESET					('F')
/**
*--------------      rfid tag type predefine     ---------------------
*/
#define CARD_TAG_TYPE_KEY_RING				('Q')
#define CARD_TAG_TYPE_CLASIC				('C')
#define CARD_TAG_TYPE_WRIST					('W')
/**
*--------------      card data define    ---------------------
*/
#define CARD_ID_INVALID                     ('!')
#define CARD_ID_INVALID_DATA                ('"')
#define CARD_USER_GROUP_INVALID             ('?')	
#define CARD_USER_GROUP_DATA_INVALID        ('$')
#define CARD_EXPIRY_TIME_INVALID            ('%')
#define CARD_EXPIRY_TIME_DATA_INVALID       ('&')
#define CARD_USAGE_TYPE_INVALID             ('/')
#define CARD_USAGE_TYPE_DATA_INVALID        ('(')
#define CARD_NUMBER_OF_USERS_INVALID        (')')
#define CARD_NUMBER_OF_USERS_DATA_INVALID   ('=')

#define MIFARE_16_BYTES                     0x01
#define MIFARE_64_BYTES                     0x02
#define MIFARE_512_BYTES                    0x03
#define MIFARE_1K_BYTES                     0x04
#define MIFARE_4K_BYTES                     0x05

#define RC522_DUMMY                         0x00    //Dummy byte
#define RC522_MAX_LEN                       8      // FIFO & buffer byte lenght


/* mifare card memory sectors offset */
#define SECTOR_0                        0x00
#define SECTOR_1                        0x04
#define SECTOR_2                        0x08
#define SECTOR_3                        0x0c
#define SECTOR_4                        0x10
#define SECTOR_5                        0x14
#define SECTOR_6                        0x18
#define SECTOR_7                        0x1c
#define SECTOR_8                        0x20
#define SECTOR_9                        0x24
#define SECTOR_10                       0x28
#define SECTOR_11                       0x2c
#define SECTOR_12                       0x30
#define SECTOR_13                       0x34
#define SECTOR_14                       0x38
#define SECTOR_15                       0x3c
#define SECTOR_16                       0x40
#define SECTOR_17                       0x44
#define SECTOR_18                       0x48
#define SECTOR_19                       0x4c
#define SECTOR_20                       0x50
#define SECTOR_21                       0x54
#define SECTOR_22                       0x58
#define SECTOR_23                       0x5c
#define SECTOR_24                       0x60
#define SECTOR_25                       0x64
#define SECTOR_26                       0x6c
#define SECTOR_27                       0x70
#define SECTOR_28                       0x74
#define SECTOR_29                       0x78
#define SECTOR_30                       0x7c
#define SECTOR_31                       0x80
#define SECTOR_X                        0xff

/* RC522 Commands */
#define PCD_IDLE                        0x00   //NO action; Cancel the current command
#define PCD_AUTHENT                     0x0E   //Authentication Key
#define PCD_RECEIVE                     0x08   //Receive Data
#define PCD_TRANSMIT                    0x04   //Transmit data
#define PCD_TRANSCEIVE                  0x0C   //Transmit and receive data,
#define PCD_RESETPHASE                  0x0F   //Reset
#define PCD_CALCCRC                     0x03   //CRC Calculate

/* Mifare_One card command word */
#define PICC_REQIDL                     0x26   // find the antenna area does not enter hibernation
#define PICC_REQALL                     0x52   // find all the cards antenna area
#define PICC_ANTICOLL                   0x93   // anti-collision
#define PICC_SELECTTAG                  0x93   // election card
#define PICC_AUTHENT1A                  0x60   // authentication key A
#define PICC_AUTHENT1B                  0x61   // authentication key B
#define PICC_READ                       0x30   // Read Block
#define PICC_WRITE                      0xA0   // write block
#define PICC_DECREMENT                  0xC0   // debit
#define PICC_INCREMENT                  0xC1   // recharge
#define PICC_RESTORE                    0xC2   // transfer block data to the buffer
#define PICC_TRANSFER                   0xB0   // save the data in the buffer
#define PICC_HALT                       0x50   // Sleep

/* RC522 Registers */
//Page 0: Command and Status
#define RC522_REG_RESERVED00			0x00
#define RC522_REG_COMMAND               0x01
#define RC522_REG_COMM_IE_N             0x02
#define RC522_REG_DIV1_EN               0x03
#define RC522_REG_COMM_IRQ              0x04
#define RC522_REG_DIV_IRQ               0x05
#define RC522_REG_ERROR                 0x06
#define RC522_REG_STATUS1               0x07
#define RC522_REG_STATUS2               0x08
#define RC522_REG_FIFO_DATA             0x09
#define RC522_REG_FIFO_LEVEL			0x0A
#define RC522_REG_WATER_LEVEL			0x0B
#define RC522_REG_CONTROL               0x0C
#define RC522_REG_BIT_FRAMING			0x0D
#define RC522_REG_COLL                  0x0E
#define RC522_REG_RESERVED01			0x0F
//Page 1: Command
#define RC522_REG_RESERVED10			0x10
#define RC522_REG_MODE                  0x11
#define RC522_REG_TX_MODE               0x12
#define RC522_REG_RX_MODE               0x13
#define RC522_REG_TX_CONTROL			0x14
#define RC522_REG_TX_AUTO               0x15
#define RC522_REG_TX_SELL               0x16
#define RC522_REG_RX_SELL               0x17
#define RC522_REG_RX_THRESHOLD			0x18
#define RC522_REG_DEMOD                 0x19
#define RC522_REG_RESERVED11			0x1A
#define RC522_REG_RESERVED12			0x1B
#define RC522_REG_MIFARE                0x1C
#define RC522_REG_RESERVED13			0x1D
#define RC522_REG_RESERVED14			0x1E
#define RC522_REG_SERIALSPEED			0x1F
//Page 2: CFG
#define RC522_REG_RESERVED20			0x20
#define RC522_REG_CRC_RESULT_M			0x21
#define RC522_REG_CRC_RESULT_L			0x22
#define RC522_REG_RESERVED21			0x23
#define RC522_REG_MOD_WIDTH             0x24
#define RC522_REG_RESERVED22			0x25
#define RC522_REG_RF_CFG                0x26
#define RC522_REG_GS_N                  0x27
#define RC522_REG_CWGS_PREG             0x28
#define RC522_REG__MODGS_PREG			0x29
#define RC522_REG_T_MODE                0x2A
#define RC522_REG_T_PRESCALER			0x2B
#define RC522_REG_T_RELOAD_H			0x2C
#define RC522_REG_T_RELOAD_L			0x2D
#define RC522_REG_T_COUNTER_VALUE_H		0x2E
#define RC522_REG_T_COUNTER_VALUE_L		0x2F
//Page 3:TestRegister
#define RC522_REG_RESERVED30			0x30
#define RC522_REG_TEST_SEL1             0x31
#define RC522_REG_TEST_SEL2             0x32
#define RC522_REG_TEST_PIN_EN			0x33
#define RC522_REG_TEST_PIN_VALUE		0x34
#define RC522_REG_TEST_BUS              0x35
#define RC522_REG_AUTO_TEST             0x36
#define RC522_REG_VERSION               0x37
#define RC522_REG_ANALOG_TEST			0x38
#define RC522_REG_TEST_ADC1             0x39
#define RC522_REG_TEST_ADC2             0x3A
#define RC522_REG_TEST_ADC0             0x3B
#define RC522_REG_RESERVED31			0x3C
#define RC522_REG_RESERVED32			0x3D
#define RC522_REG_RESERVED33			0x3E
#define RC522_REG_RESERVED34			0x3F

#define RC522_IDLE                      0x00
#define RC522_READ                      0x01
#define RC522_WRITE                     0x02
#define RC522_WRITE_KEY                 0x03

/* Exported types    ---------------------------------------------------------*/

typedef struct
{
    unsigned char B_Dat_0[16];
    unsigned char  B_Dat_1[16];
    unsigned char  B_Dat_2[16];

} RC522_SectorTypeDef;


extern RC522_SectorTypeDef Sector;


/* Exported variables  -------------------------------------------------------*/
extern unsigned char ReceivedDataBuffer[64];
extern unsigned char ToSendDataBuffer[64];
//extern BYTE rc522_config;
//extern volatile DWORD mifare_process_timer;
//extern volatile DWORD mifare_process_flags;
//
//extern BYTE aMifareAuthenticationKeyA[6];
//extern BYTE aMifareAuthenticationKeyB[6];
//extern BYTE aCardUserGroup[16];
//extern BYTE aCardUsageType[16];
//extern BYTE card_max_users_cnt;

//extern RC522_CardDataTypeDef sCardDataMifare1;


/* Exported macros     -------------------------------------------------------*/


/* Exported functions  -------------------------------------------------------*/
extern void RC522_Init(void);
extern void RC522_Service(void);
extern void RC522_ClearData(void);

#endif