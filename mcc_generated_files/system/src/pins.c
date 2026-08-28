/**
 * Generated Driver File
 * 
 * @file pins.c
 * 
 * @ingroup  pinsdriver
 * 
 * @brief This is generated driver implementation for pins. 
 *        This file provides implementations for pin APIs for all pins selected in the GUI.
 *
 * @version Driver Version 3.0.0
*/

/*
© [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#include "../pins.h"

void (*POWER_InterruptHandler)(void);
void (*VOL_A_InterruptHandler)(void);
void (*VOL_B_InterruptHandler)(void);

void PIN_MANAGER_Initialize(void)
{
   /**
    LATx registers
    */
    LATA = 0x0;
    LATB = 0x0;
    LATC = 0x0;
    LATD = 0x0;
    LATE = 0x0;

    /**
    TRISx registers
    */
    TRISA = 0xDD;
    TRISB = 0xEF;
    TRISC = 0xFF;
    TRISD = 0xCD;
    TRISE = 0xF;

    /**
    ANSELx registers
    */
    ANSELA = 0xD5;
    ANSELB = 0xCD;
    ANSELC = 0xA7;
    ANSELD = 0xCB;
    ANSELE = 0x7;

    /**
    WPUx registers
    */
    WPUA = 0x0;
    WPUB = 0x0;
    WPUC = 0x0;
    WPUD = 0x0;
    WPUE = 0x0;
  
    /**
    ODx registers
    */
   
    ODCONA = 0x0;
    ODCONB = 0x0;
    ODCONC = 0x0;
    ODCOND = 0x0;
    ODCONE = 0x0;
    /**
    SLRCONx registers
    */
    SLRCONA = 0xFF;
    SLRCONB = 0xFF;
    SLRCONC = 0xFF;
    SLRCOND = 0xFF;
    SLRCONE = 0x7;
    /**
    INLVLx registers
    */
    INLVLA = 0xFF;
    INLVLB = 0xFF;
    INLVLC = 0xFF;
    INLVLD = 0xFF;
    INLVLE = 0xF;

    /**
    PPS registers
    */
    RX2PPS = 0x1A; //RD2->EUSART2:RX2;
    RX1PPS = 0xD; //RB5->EUSART1:RX1;
    RD4PPS = 0x0B;  //RD4->PWM3:PWM3;
    RD5PPS = 0x0C;  //RD5->PWM4:PWM4;
    RD1PPS = 0x11;  //RD1->EUSART2:TX2;
    RB4PPS = 0x0E;  //RB4->EUSART1:TX1;

    /**
    APFCON registers
    */

   /**
    IOCx registers 
    */
    IOCAP = 0x0;
    IOCAN = 0x8;
    IOCAF = 0x0;
    IOCBP = 0x0;
    IOCBN = 0x0;
    IOCBF = 0x0;
    IOCCP = 0x18;
    IOCCN = 0x18;
    IOCCF = 0x0;
    IOCEP = 0x0;
    IOCEN = 0x0;
    IOCEF = 0x0;

    POWER_SetInterruptHandler(POWER_DefaultInterruptHandler);
    VOL_A_SetInterruptHandler(VOL_A_DefaultInterruptHandler);
    VOL_B_SetInterruptHandler(VOL_B_DefaultInterruptHandler);

    // Enable PIE0bits.IOCIE interrupt 
    PIE0bits.IOCIE = 1; 
}
  
void PIN_MANAGER_IOC(void)
{
    // interrupt on change for pin POWER}
    if(IOCAFbits.IOCAF3 == 1)
    {
        POWER_ISR();  
    }
    // interrupt on change for pin VOL_A}
    if(IOCCFbits.IOCCF3 == 1)
    {
        VOL_A_ISR();  
    }
    // interrupt on change for pin VOL_B}
    if(IOCCFbits.IOCCF4 == 1)
    {
        VOL_B_ISR();  
    }
}
   
/**
   POWER Interrupt Service Routine
*/
void POWER_ISR(void) {

    // Add custom IOCAF3 code

    // Call the interrupt handler for the callback registered at runtime
    if(POWER_InterruptHandler)
    {
        POWER_InterruptHandler();
    }
    IOCAFbits.IOCAF3 = 0;
}

/**
  Allows selecting an interrupt handler for IOCAF3 at application runtime
*/
void POWER_SetInterruptHandler(void (* InterruptHandler)(void)){
    POWER_InterruptHandler = InterruptHandler;
}

/**
  Default interrupt handler for IOCAF3
*/
void POWER_DefaultInterruptHandler(void){
    // add your POWER interrupt custom code
    // or set custom function using POWER_SetInterruptHandler()
}
   
/**
   VOL_A Interrupt Service Routine
*/
void VOL_A_ISR(void) {

    // Add custom IOCCF3 code

    // Call the interrupt handler for the callback registered at runtime
    if(VOL_A_InterruptHandler)
    {
        VOL_A_InterruptHandler();
    }
    IOCCFbits.IOCCF3 = 0;
}

/**
  Allows selecting an interrupt handler for IOCCF3 at application runtime
*/
void VOL_A_SetInterruptHandler(void (* InterruptHandler)(void)){
    VOL_A_InterruptHandler = InterruptHandler;
}

/**
  Default interrupt handler for IOCCF3
*/
void VOL_A_DefaultInterruptHandler(void){
    // add your VOL_A interrupt custom code
    // or set custom function using VOL_A_SetInterruptHandler()
}
   
/**
   VOL_B Interrupt Service Routine
*/
void VOL_B_ISR(void) {

    // Add custom IOCCF4 code

    // Call the interrupt handler for the callback registered at runtime
    if(VOL_B_InterruptHandler)
    {
        VOL_B_InterruptHandler();
    }
    IOCCFbits.IOCCF4 = 0;
}

/**
  Allows selecting an interrupt handler for IOCCF4 at application runtime
*/
void VOL_B_SetInterruptHandler(void (* InterruptHandler)(void)){
    VOL_B_InterruptHandler = InterruptHandler;
}

/**
  Default interrupt handler for IOCCF4
*/
void VOL_B_DefaultInterruptHandler(void){
    // add your VOL_B interrupt custom code
    // or set custom function using VOL_B_SetInterruptHandler()
}
/**
 End of File
*/