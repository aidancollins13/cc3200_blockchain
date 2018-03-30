	//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Blinky
// Application Overview - The objective of this application is to showcase the 
//                        GPIO control using Driverlib api calls. The LEDs 
//                        connected to the GPIOs on the LP are used to indicate 
//                        the GPIO output. The GPIOs are driven high-low 
//                        periodically in order to turn on-off the LEDs.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Blinky_Application
// or
// docs\examples\CC32xx_Blinky_Application.pdf
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup blinky
//! @{
//
//****************************************************************************

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
volatile bool g_bContextReadyFlag;
volatile bool g_bParthashReadyFlag;
volatile bool g_bInputReadyFlag;
volatile bool g_bOutputReadyFlag;


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
	char pHash[32];
	char hash[32];
	char data[32];
};

struct Block* gen_block(struct Block* lastb, char* data ){

	struct Block *b = malloc(sizeof(struct Block));

	b->index = lastb->index +1;
	strcpy(b->hash, lastb->hash);
	//strcpy(b.hahs, GenHash());
	strcpy(b->data, data);
	return b;

}




unsigned char *result;
unsigned char* data;
unsigned char *puiKey1;
unsigned int uiDataLength;
unsigned int u8count;


unsigned int iSize, uiMsgLen, uiConfig, uiHashLength;
int
main()
{

    //
    // Initialize Board configurations
    //
    BoardInit();
    
    //
    // Power on the corresponding GPIO port B for 9,10,11.
    // Set up the GPIO lines to mode 0 (GPIO)
    //
    PinMuxConfig();
    InitTerm();
    MAP_PRCMPeripheralClkEnable(PRCM_DTHE, PRCM_RUN_MODE_CLK);
      //
      // Enable interrupts.
      //
    UART_PRINT("testing\n\r");
    MAP_SHAMD5IntRegister(SHAMD5_BASE, SHAMD5IntHandler);

    UART_PRINT("enabled int\n\r");
    uiConfig=SHAMD5_ALGO_SHA256;
    uiHashLength=32;

    data=(unsigned char *)malloc(5);
    memset(data,0,5);
    memcpy(data,"test",(4));

    result=(unsigned char *)malloc(64);
                memset(result,0,64);
    UART_PRINT("loaded sha256 settings\n\r");
    UART_PRINT("Generating Hash\n\r");


    UART_PRINT("data %s\n\r",data);
    GenerateHash(uiConfig, data,  result, 4);
    for(u8count=0;u8count<uiHashLength;u8count++)
           {
             UART_PRINT("%02x",*(result+u8count));
           }
           UART_PRINT("\n\r");
    UART_PRINT("end of main\n\r");
    return 0;
}
