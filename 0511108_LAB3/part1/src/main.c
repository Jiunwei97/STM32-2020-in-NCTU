#include "stm32l476xx.h"
#include "helper_functions.h"
#include "led_button.h"
#include "7seg.h"

// Define pins for 4 leds
//#define LED_gpio GPIOA
//#define LED1_pin 5
//#define LED2_pin 6
//#define LED3_pin 7
//#define LED4_pin 8

// Define pins for button (default use on-board button PC13)
#define BUTTON_gpio GPIOC
#define BUTTON_pin 13

// Define pins for 7seg
#define SEG_gpio GPIOB
#define DIN_pin 3
#define CS_pin 4
#define CLK_pin 5

int main(){
	if(init_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin) != 0){
		// Fail to init 7seg
		return -1;
	}
	int display_number(GPIO_TypeDef* gpio, int DIN, int CS, int CLK , int num, int num_digs){
		for (int i=1;i<=num_digs;i++){
			send_7seg(gpio,DIN,CS,CLK,i,num%10);
			num/=10;
		}
		for (int i=num_digs+1;i<=8;i++){
			num/=10;
			send_7seg(gpio, DIN,CS,CLK,i,15);
		}
		if(num!=0)
			return -1;
		return 0;
	}
	// Set Decode Mode
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_DECODE_MODE, 0xFF);
	// Set Scan Limit to digit 0 only
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SCAN_LIMIT, 0x06);
	// Wakeup 7seg
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SHUTDOWN, 0x01);

	int studentid=511108;
	while(1){
		int number=511108;
		int number_digs= 7;
		display_number(SEG_gpio,DIN_pin, CS_pin, CLK_pin, number, number_digs);

	}

	return 0;
}
