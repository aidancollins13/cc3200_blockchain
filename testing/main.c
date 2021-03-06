
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Driverlib includes
#include "hw_shamd5.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_common_reg.h"
#include "rom.h"
#include "rom_map.h"
#include "shamd5.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"
#include "utils.h"
#include "shamd5.h"

// Common interface includes
#include "uart_if.h"

#include "pinmux.h"
#include "shamd5_vector.h"
#include "shamd5_userinput.h"

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
#define UART_PRINT           Report
struct Block s;
static void BoardInit(void);
void SetKeys(void);
void GenerateHash(unsigned int uiConfig,unsigned char
        *puiData,unsigned char *puiResult,unsigned int uiDataLength);
void SHAMD5IntHandler(void);
int check_block(struct Block *b);

volatile bool g_bContextReadyFlag;
volatile bool g_bParthashReadyFlag;
volatile bool g_bInputReadyFlag;
volatile bool g_bOutputReadyFlag;222


static void
BoardInit(void)
{
	#ifndef USE_TIRTOS
	  //
	  // Set vector table base
	  //
	#if defined(ccs)
	    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
	#endif
	#if defined(ewarm)
	    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
	#endif
	#endif

    //
    // Enable Processor
    //

    IntMasterEnable();
    IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//!
//! \return None.
//
//***************************************************************************

void
GenerateHash(unsigned int uiConfig,unsigned char *puiData,
        unsigned char *puiResult,unsigned int uiDataLength)
{

	UART_PRINT("HASHING: %s\r\n", puiData);
    //
    // Step1: Enable Interrupts
    // Step2: Wait for Context Ready Inteerupt
    // Step3: Set the Configuration Parameters (Hash Algorithm)
    // Step4: Set Key depends on Algorithm
    // Step5: Start Hash Generation
    //

    //
    // Clear the flags
    //
    g_bContextReadyFlag = false;
    g_bInputReadyFlag = false;
    //
    // Enable interrupts.
    //
    MAP_SHAMD5IntEnable(SHAMD5_BASE, SHAMD5_INT_CONTEXT_READY |
                    SHAMD5_INT_PARTHASH_READY |
                    SHAMD5_INT_INPUT_READY |
                    SHAMD5_INT_OUTPUT_READY);
    //
    // Wait for the context ready flag.
    //
    while(!g_bContextReadyFlag)
    {
    }
    //
    // Configure the SHA/MD5 module.
    //
    MAP_SHAMD5ConfigSet(SHAMD5_BASE, uiConfig);


    //
    // Perform the hashing operation
    //
    MAP_SHAMD5DataProcess(SHAMD5_BASE, puiData, uiDataLength,
                          puiResult);



}


void
SHAMD5IntHandler(void)
{
    uint32_t ui32IntStatus;
    //
    // Read the SHA/MD5 masked interrupt status.
    //
    ui32IntStatus = MAP_SHAMD5IntStatus(SHAMD5_BASE, true);
    if(ui32IntStatus & SHAMD5_INT_CONTEXT_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_CONTEXT_READY);
        g_bContextReadyFlag = true;

    }
    if(ui32IntStatus & SHAMD5_INT_PARTHASH_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_PARTHASH_READY);
        g_bParthashReadyFlag=true;

    }
    if(ui32IntStatus & SHAMD5_INT_INPUT_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_INPUT_READY);
        g_bInputReadyFlag = true;

    }
    if(ui32IntStatus & SHAMD5_INT_OUTPUT_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_OUTPUT_READY);
        g_bOutputReadyFlag = true;

    }

}

struct Block{
	int index;
	unsigned char pHash[32];
	unsigned char hash[32];
	unsigned char data[10];
};



struct Block* gen_block(struct Block* lastb, char* data ){

	struct Block *b = malloc(sizeof(struct Block));
	//char data;
	// prep the data
	unsigned char d[42];
	strcpy(d, lastb->hash);
	strcpy(d+32, data);
	memset(d+42, 0,1);
	b->index = lastb->index +1;
	strcpy(b->hash, lastb->hash);
	GenerateHash(SHAMD5_ALGO_SHA256, d, b->hash, 42);
	strcpy(b->data, d);
	return b;

}

struct Block * gen_genesis_block(){
	struct Block *b = malloc(sizeof(struct Block));
	strcpy(b->data, "000000000");
	GenerateHash(SHAMD5_ALGO_SHA256, "genesis block", b->hash, 13);
	b->index = 0;
	return b;


}

int verify_block(struct Block * block, struct Block* lastb){
	unsigned char h[32];
	unsigned char d[32];
	strcpy(d, lastb->hash);
	strcpy(d+32, block->data);
	memset(d+42, 0, 1);
	GenerateHAsh(SHAMD5_ALGO_SHA256,d,h,32);

	return strcmp(h, block->hash) && (block->index == lastb->index + 1);
}


unsigned char *result;
unsigned char* data;
unsigned char *puiKey1;
unsigned int uiDataLength;
unsigned int u8count;
struct Block *blocks[10];


unsigned int iSize, uiMsgLen, uiConfig, uiHashLength;
int
main()
{

    //
    // Initialize Board configurations
    //
    BoardInit();
    UDMAInit();
    PinMuxConfig();
    InitTerm();
    MAP_PRCMPeripheralClkEnable(PRCM_DTHE, PRCM_RUN_MODE_CLK);

    // Set up wireless connection

      //
      // Enable interrupts.
      //
    UART_PRINT("testing\n\r");
    MAP_SHAMD5IntRegister(SHAMD5_BASE, SHAMD5IntHandler);

    UART_PRINT("enabled int\n\r");
    blocks[0] = gen_genesis_block();

//    uiConfig=SHAMD5_ALGO_SHA256;
//    uiHashLength=32;
//
//    data=(unsigned char *)malloc(5);
//    memset(data,0,5);
//    memcpy(data,"test",(4));
//
//    result=(unsigned char *)malloc(64);
//                memset(result,0,64);
//    UART_PRINT("loaded sha256 settings\n\r");
//    UART_PRINT("Generating Hash\n\r");
//
//
//    UART_PRINT("data %s\n\r",data);
//    GenerateHash(uiConfig, "test",  result, 4);
    for(u8count=0;u8count<32;u8count++)
           {
             UART_PRINT("%02x",*(blocks[0]->hash + u8count));
           }
           UART_PRINT("\n\r");

    blocks[1] = gen_block(blocks[0], "test00000");
    UART_PRINT("block1 data: %s\r\n", blocks[1]->data);
    UART_PRINT("block1 last hash: %s\r\n", blocks[1]->pHash);

    for(u8count=0;u8count<32;u8count++)
               {
                 UART_PRINT("%02x",*(blocks[1]->hash + u8count));
               }
               UART_PRINT("\n\r");

    UART_PRINT("end of main\n\r");
    return 0;
}
