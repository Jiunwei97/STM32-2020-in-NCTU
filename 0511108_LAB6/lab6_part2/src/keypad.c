#include "keypad.h"

// Mapping for key-press
const int keypad[4][4] = {
	{1, 2, 3, 10},
	{4, 5, 6, 11},
	{7, 8, 9, 12},
	{15, 11, 14, 13}
};

// Only allow GPIOA and GPIOB for now
// Can easily extended by adding "else if" cases
int init_keypad(GPIO_TypeDef* ROW_gpio, GPIO_TypeDef* COL_gpio, int ROW_pin, int COL_pin){
	// Enable AHB2 Clock
	if(ROW_gpio==GPIOA || COL_gpio==GPIOA){
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	}
	if(ROW_gpio==GPIOB || COL_gpio==GPIOB){
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	}

	// First Clear bits(&) then set bits(|)
	for(int a=0;a<4;a++){
		// Set GPIO pins to output mode (01)
		COL_gpio->MODER &= ~(0b11 << (2*(COL_pin+a)));
		COL_gpio->MODER |= (0b01 << (2*(COL_pin+a)));
		// Set GPIO pins to open drain mode (1)
		COL_gpio->OTYPER &= ~(0b1 << (COL_pin+a));
		COL_gpio->OTYPER |= (0b1 << (COL_pin+a));
		// Set Output to high
		set_gpio(COL_gpio, COL_pin+a);
	}

	// First Clear bits(&) then set bits(|)
	for(int a=0;a<4;a++){
		// Set GPIO pins to input mode (00)
		ROW_gpio->MODER &= ~(0b11 << (2*(ROW_pin+a)));
		ROW_gpio->MODER |= (0b00 << (2*(ROW_pin+a)));
		// Set GPIO pins to Pull-Down mode (10)
		ROW_gpio->PUPDR &= ~(0b11 << (2*(ROW_pin+a)));
		ROW_gpio->PUPDR |= (0b10 << (2*(ROW_pin+a)));
	}

	return 0;
}

int check_keypad_input_one(GPIO_TypeDef* ROW_gpio, GPIO_TypeDef* COL_gpio, int ROW_pin, int COL_pin, int x, int y){
	int cycles = 400;
	// Set Column to push-pull mode
	COL_gpio->OTYPER &= ~(1 << (COL_pin+y));
	// Count the total number of time it is pressed in a certain period
	int cnt = 0;
	for(int a=0;a<cycles;a++){
		cnt += read_gpio(ROW_gpio, ROW_pin+x);
	}
	// Set Column back to open drain mode
	COL_gpio->OTYPER |= (1 << (COL_pin+y));
	// return if the key is pressed(1) or not(0)
	return (cnt > (cycles*0.7));
}

