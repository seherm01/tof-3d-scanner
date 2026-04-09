/*

Modified: Apr 6, 2026
Seher Mian

Hallway: J
Assigned Bus speed = 36 MHz
LEDS:
Measurement: PN0 - D2
UART tx: PN1 - D1
Additional Status PF0 - D4 - turns on when wires are being untwisted




UART - no clock, rx and tx, reciever and transmitter
I2C - two lines, scl - clock line, sda - data line

PC to MCU - UART
MCU to ToF - I2C


*/


#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"

uint16_t dev = 0x29;
int status = 0;


// Initialize Ports

void PortJ_Init(void){ // ON board switches
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R8) == 0){}

    GPIO_PORTJ_DIR_R &= ~0x03; // PJ0 and PJ1 INPUT
    GPIO_PORTJ_DEN_R |= 0x03;
    GPIO_PORTJ_PUR_R |= 0x03;
    GPIO_PORTJ_AFSEL_R &= ~0x03;
    GPIO_PORTJ_AMSEL_R &= ~0x03;
}

void PortL_Init(void){ // For motor output
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R10;
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R10) == 0){}

    GPIO_PORTL_DIR_R |= 0x0F;
    GPIO_PORTL_DEN_R |= 0x0F;
    GPIO_PORTL_AFSEL_R &= ~0x0F;
    GPIO_PORTL_AMSEL_R &= ~0x0F;
}

void PortN_Init(void){ // PN0 and PN1 (Status LEDs)
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0){}

    GPIO_PORTN_DIR_R |= 0x03;
    GPIO_PORTN_DEN_R |= 0x03;
    GPIO_PORTN_DATA_R &= ~0x03;
}

void PortF_Init(void){ // PF0 - D4
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0){}

    GPIO_PORTF_DIR_R |= 0x01;
    GPIO_PORTF_DEN_R |= 0x01;
    GPIO_PORTF_DATA_R &= ~0x01;
}

void I2C_Init(void){
	// enable clock
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};		// wait until port B is ready																// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
   // I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
		I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}



//LED CONTROL

// SET D1
void SetMeasurement_On(void){ GPIO_PORTN_DATA_R |= 0x01; } // turn d1 on to indicate measurement in progress
void SetMeasurement_Off(void){ GPIO_PORTN_DATA_R &= ~0x01; } // tuen off d1

// Pulse UART LED
void PulseUARTTx(void){
    GPIO_PORTN_DATA_R |= 0x02;
    SysTick_Wait10ms(2);
    GPIO_PORTN_DATA_R &= ~0x02;
}

// SET D4 (additional status on when untwisting wires)
void SetAdditionalStatus_On(void){ GPIO_PORTF_DATA_R |= 0x01; }
void SetAdditionalStatus_Off(void){ GPIO_PORTF_DATA_R &= ~0x01; }


// BUTTONS CONTROL

void WaitForButtonPress(void){ // PJ0
    while((GPIO_PORTJ_DATA_R & 0x01) != 0){}
    SysTick_Wait10ms(2);
    while((GPIO_PORTJ_DATA_R & 0x01) == 0){}
    SysTick_Wait10ms(2);
}

// PJ1
void WaitForPJ1Press(void){
    while((GPIO_PORTJ_DATA_R & 0x02) != 0){}
    SysTick_Wait10ms(2);
    while((GPIO_PORTJ_DATA_R & 0x02) == 0){}
    SysTick_Wait10ms(2);
}

// MAIN FUNCTION

int main(void){

	
		// variables
    uint16_t Distance, SignalRate, AmbientRate, SpadNum;
    uint8_t RangeStatus, dataReady = 0, sensorState = 0;
    uint16_t wordData;

    // Initialize
    
    PLL_Init();
    SysTick_Init();
    onboardLEDs_Init();
    I2C_Init();
    UART_Init();
    PortL_Init();
    PortJ_Init();
		PortN_Init();
    PortF_Init();

    UART_printf("Program Begins\r\n");

    VL53L1X_GetSensorId(dev, &wordData);

		// wait for sensor data ready
    while(sensorState == 0){
        VL53L1X_BootState(dev, &sensorState);
        SysTick_Wait10ms(10);
    }

    VL53L1X_ClearInterrupt(dev);
    VL53L1X_SensorInit(dev);

    // Wait for start
    UART_printf("Press PJ0 to start...\r\n");
    WaitForButtonPress();
 
    int scan = 0;

    while(1){

        scan++;

        UART_printf("START_SCAN\r\n");
        PulseUARTTx();

        VL53L1X_StartRanging(dev);

        // Scan
        for(int i = 0; i < 32; i++){

            dataReady = 0;
            while(dataReady == 0){
                VL53L1X_CheckForDataReady(dev, &dataReady);
                VL53L1_WaitMs(dev, 5);
            }

            for(int j = 0; j < 16; j++){
                GPIO_PORTL_DATA_R = 0b00000011; SysTick_Wait10ms(2);
                GPIO_PORTL_DATA_R = 0b00000110; SysTick_Wait10ms(2);
                GPIO_PORTL_DATA_R = 0b00001100; SysTick_Wait10ms(2);
                GPIO_PORTL_DATA_R = 0b00001001; SysTick_Wait10ms(2);
            }

            SetMeasurement_On();

            VL53L1X_GetRangeStatus(dev, &RangeStatus);
            VL53L1X_GetDistance(dev, &Distance);
            VL53L1X_GetSignalRate(dev, &SignalRate);
            VL53L1X_GetAmbientRate(dev, &AmbientRate);
            VL53L1X_GetSpadNb(dev, &SpadNum);

						SysTick_Wait10ms(2);
            SetMeasurement_Off();

            VL53L1X_ClearInterrupt(dev);

            sprintf(printf_buffer,"%d,%u,%u,%u,%u,%u\r\n",
                    i+1, Distance, RangeStatus, SignalRate, AmbientRate, SpadNum);

            UART_printf(printf_buffer);
            PulseUARTTx();
        }

        VL53L1X_StopRanging(dev);

        UART_printf("END_SCAN\r\n");

        // Untwist wires (ccw)
				SetAdditionalStatus_On();
        for(int i = 0; i < 32; i++){
            for(int j = 0; j < 16; j++){
                GPIO_PORTL_DATA_R = 0b00001001; SysTick_Wait10ms(2);
                GPIO_PORTL_DATA_R = 0b00001100; SysTick_Wait10ms(2);
                GPIO_PORTL_DATA_R = 0b00000110; SysTick_Wait10ms(2);
                GPIO_PORTL_DATA_R = 0b00000011; SysTick_Wait10ms(2);
            }
        }
				SetAdditionalStatus_Off();
        GPIO_PORTL_DATA_R = 0x00; // MOTOR OFF

        // Wait for user
        UART_printf("WAITING\r\n");

        while(1){

            // PJ1 --> another scan
            if((GPIO_PORTJ_DATA_R & 0x02) == 0){
                WaitForPJ1Press();
                SetAdditionalStatus_Off();
                break;
            }

            // PJ0 --> stop scanning
            if((GPIO_PORTJ_DATA_R & 0x01) == 0){
                WaitForButtonPress();

                GPIO_PORTL_DATA_R = 0x00;
                SetAdditionalStatus_Off();

                UART_printf("DONE\r\n");

                while(1);
            }
        }
    }
}
