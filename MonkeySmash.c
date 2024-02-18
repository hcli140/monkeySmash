#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "SysTick.h"

volatile uint32_t ADCvalue;

//-ADC0_InSeq3-
// Busy-wait analog to digital conversion. 0 to 3.3V maps to 0 to 4095 
// Input: none 
// Output: 12-bit result of ADC conversion 
uint32_t ADC0_InSeq3(void){
	uint32_t result;
	
	ADC0_PSSI_R = 0x0008;														// 1) initiate SS3   
	while((ADC0_RIS_R&0x08)==0){}										// 2) wait for conversion done   
	result = ADC0_SSFIFO3_R&0xFFF;									// 3) read 12-bit result   
	ADC0_ISC_R = 0x0008;														// 4) acknowledge completion   
	
	return result; 
}

void ADC_Init(void){
	//config the ADC from Valvano textbook
	SYSCTL_RCGCGPIO_R |= 0b00010000;								// 1. activate clock for port E
	while ((SYSCTL_PRGPIO_R & 0b00010000) == 0) {}	//		wait for clock/port to be ready
	GPIO_PORTE_DIR_R &= ~0x10;											// 2) make PE4 input   
	GPIO_PORTE_AFSEL_R |= 0x10;											// 3) enable alternate function on PE4   
	GPIO_PORTE_DEN_R &= ~0x10;											// 4) disable digital I/O on PE4   
	GPIO_PORTE_AMSEL_R |= 0x10;											// 5) enable analog function on PE4   
	SYSCTL_RCGCADC_R |= 0x01;												// 6) activate ADC0   
	ADC0_PC_R = 0x01;																// 7) maximum speed is 125K samples/sec   
	ADC0_SSPRI_R = 0x0123;													// 8) Sequencer 3 is highest priority   
	ADC0_ACTSS_R &= ~0x0008;												// 9) disable sample sequencer 3   
	ADC0_EMUX_R &= ~0xF000;													// 10) seq3 is software trigger 
	ADC0_SSMUX3_R = 9;															// 11) set channel Ain9 (PE4)   
	ADC0_SSCTL3_R = 0x0006;													// 12) no TS0 D0, yes IE0 END0   
	ADC0_IM_R &= ~0x0008;														// 13) disable SS3 interrupts   
	ADC0_ACTSS_R |= 0x0008;													// 14) enable sample sequencer 3 
	//==============================================================================	
	
	return;
}

void PortN_Init(void){
	//Use PortN onboard LED	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};
	GPIO_PORTN_DIR_R |= 0x03;
	GPIO_PORTN_AFSEL_R &= ~0x03;
  GPIO_PORTN_DEN_R |= 0x03;
  GPIO_PORTN_AMSEL_R &= ~0x01;
	return;
}

void PortM_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};
	GPIO_PORTM_DIR_R &= ~0x0F;
  GPIO_PORTM_DEN_R |= 0x0F;
	//GPIO_PORTM_AFSEL_R &= ~0x0F;
  //GPIO_PORTM_AMSEL_R &= ~0x0F;
	return;
}

void PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};
	GPIO_PORTF_DIR_R |= 0x11;
  GPIO_PORTF_DEN_R |= 0x11;
	GPIO_PORTF_AFSEL_R &= ~0x11;
  GPIO_PORTF_AMSEL_R &= ~0x11;
	return;
}

void D1_ON() {
	GPIO_PORTN_DATA_R |= 0b10;
}

void D2_ON() {
	GPIO_PORTN_DATA_R |= 0b1;
}

void D3_ON() {
	GPIO_PORTF_DATA_R |= 0b10000;
}

void D4_ON() {
	GPIO_PORTF_DATA_R |= 0b1;
}

void D1_OFF() {
	GPIO_PORTN_DATA_R &= 0xFFFFFFFD;
}

void D2_OFF() {
	GPIO_PORTN_DATA_R &= 0xFFFFFFFE;
}

void D3_OFF() {
	GPIO_PORTF_DATA_R &= 0xFFFFFFEF;
}

void D4_OFF() {
	GPIO_PORTF_DATA_R &= 0xFFFFFFFE;
}

void all_ON() {
	D1_ON();
	D2_ON();
	D3_ON();
	D4_ON();
}

void all_OFF() {
	D1_OFF();
	D2_OFF();
	D3_OFF();
	D4_OFF();
}

uint16_t randInt0_3(uint32_t sample) {
	if (sample < 1024) return 0;
	else if (sample < 2048) return 1;
	else if (sample < 3072) return 2;
	else return 3;
}

void randLED(uint16_t led) {
	switch (led) {
		case 0:
			D1_ON();
			break;
		case 1:
			D2_ON();
			break;
		case 2:
			D3_ON();
			break;
		case 3:
			D4_ON();
			break;
	}
}


void start_sequence() {
	all_OFF();
	D1_ON();
	SysTick_Wait10ms(100);
	D2_ON();
	SysTick_Wait10ms(100);
	D3_ON();
	SysTick_Wait10ms(100);
	D4_ON();
	SysTick_Wait10ms(100);
	all_OFF();
}

uint16_t detect_false_start(uint32_t sample, uint32_t max_additional_delay) {
	uint32_t delay_ticks = sample * max_additional_delay / 4096;
	for (int i = 0; i < delay_ticks; i++) {
		if ((GPIO_PORTM_DATA_R & 0xF) != 0) return 1;
		SysTick_Wait(1);
	}
	return 0;
}

uint32_t wait_for_input() {
	uint32_t time_elapsed = 0;
	while (1) {
		if ((GPIO_PORTM_DATA_R & 0xF) != 0) break;
		SysTick_Wait(120000);
		time_elapsed++;
	}
	return time_elapsed;
}

uint16_t correct_input(uint16_t LED) {
	switch (LED) {
		case 0:
			return (GPIO_PORTM_DATA_R & 0xF) == 0b1;
		case 1:
			return (GPIO_PORTM_DATA_R & 0xF) == 0b10;
		case 2:
			return (GPIO_PORTM_DATA_R & 0xF) == 0b100;
		case 3:
			return (GPIO_PORTM_DATA_R & 0xF) == 0b1000;
		default:
			return 0;
	}
}

void close_sequence(uint16_t count) {
	for (int i = 0; i < count; i++) {
		all_OFF();
		SysTick_Wait10ms(50);
		all_ON();
		SysTick_Wait10ms(50);
	}
}




void false_sequence(uint16_t count) {
	for(int i = 0; i < count; i++){
		all_OFF();
		SysTick_Wait10ms(25);
		D1_ON();
		SysTick_Wait10ms(25);
		D1_OFF();
		D2_ON();
		SysTick_Wait10ms(25);
		D2_OFF();
		D3_ON();
		SysTick_Wait10ms(25);
		D3_OFF();
		D4_ON();
		SysTick_Wait10ms(25);
	}
	all_ON();
}

void wrong_button_sequence(uint16_t count) {
	for (int i = 0; i < count; i++) {
		D1_ON();
		D2_OFF();
		D3_ON();
		D4_OFF();
		SysTick_Wait10ms(50);
		D1_OFF();
		D2_ON();
		D3_OFF();
		D4_ON();
		SysTick_Wait10ms(50);
	}
	all_ON();
}

uint16_t TURNS = 10;

uint32_t times[10];


int main(void){
  				
	uint32_t 	min_delay = 50, max_additional_delay = 360000000, count = 0;
	uint16_t LED, false_start_flag = 0, wrong_button_flag = 0;
	
	PLL_Init();
	SysTick_Init();
	ADC_Init();
	PortN_Init();
	PortF_Init();
	PortM_Init();
	start_sequence();
	for (int count = 0; count < TURNS; count++) {
		all_OFF();
		SysTick_Wait10ms(min_delay);
		false_start_flag = detect_false_start(ADC0_InSeq3(), max_additional_delay);
		if (false_start_flag) break;
		LED = randInt0_3(ADC0_InSeq3());
		randLED(LED);
		times[count] = wait_for_input();
		wrong_button_flag = !correct_input(LED);
		if (wrong_button_flag) break;
	}
	if (false_start_flag) false_sequence(5);
	else if (wrong_button_flag) wrong_button_sequence(3);
	else close_sequence(3);
}