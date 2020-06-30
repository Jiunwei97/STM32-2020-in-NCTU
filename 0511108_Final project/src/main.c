#include "stm32l476xx.h"
#include "7seg.h"
#include "keypad.h"
#include "helper_functions.h"
#include "led_button.h"
#include "timer.h"

//Define Counter timer
#define COUNTER_timer TIM2
#define sensor_gpio GPIOC
#define sensor_pin 6

// Define pins for 7seg
#define SEG_gpio GPIOC
#define DIN_pin 3
#define CS_pin 4
#define CLK_pin 5

// Define pins for keypad
#define COL_gpio GPIOA
#define COL_pin 6      // 6 7 8 9
#define ROW_gpio GPIOB
#define ROW_pin 3       // 3 4 5 6

int main(){
	FPU_init();
	if(init_keypad(ROW_gpio, COL_gpio, ROW_pin, COL_pin) != 0){
			return -1;}
    if(initsensor(sensor_gpio,sensor_pin)!=0){
				return -1;}
	if(init_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin) != 0){
			return -1;}
	GPIO_init_AF();
	timer_enable(TIM2); //Enable timer
	timer_init(TIM2,799,99); //Init the timer//4M/800/100  <--- period
	timer_start(TIM2);

	//7-segment setting:
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_DECODE_MODE, 0xFF);
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SCAN_LIMIT, 0x07);
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SHUTDOWN, 0x01);
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_ITENSITY, 0x05);
	// Clear the digits
	for (int i=1;i<=8;i++){
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i, 15);
	}
	int B =0;
	int A =0;
	int whileswitch=1;
	int whileswitch2=0;
	restart:
	while(1){
	while(whileswitch){
		if(read_gpio(sensor_gpio, sensor_pin)==0){
			PWM_channel_init(5);
			delay_without_interrupt(500);
		}
		else {
			PWM_channel_init(10);
		}
		int save=0;
		for(int i=0;i<4;i++){
			for(int j=0;j<4;j++){
						if(check_keypad_input_one(ROW_gpio, COL_gpio, ROW_pin, COL_pin, i, j)){
						//	input = 1;
							save = keypad[i][j];
							if (save<10){
								B=save%10;
								display_number(SEG_gpio, DIN_pin, CS_pin, CLK_pin,save,num_digits(B));
							}
							//delay_without_interrupt(400);
							if(save==15){
								whileswitch=0;
								whileswitch2=1;
								send_7seg(SEG_gpio, DIN_pin, CS_pin,  CLK_pin, 3, 10);
								//send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, 1, 15);
							}
						}
					}
					}///for

		if(whileswitch2>0){
			A=B;
			for(int i=0;i<A;i++){
				delay_without_interrupt(500);
				PWM_channel_init(5);
				delay_without_interrupt(500);
				PWM_channel_init(10);
				delay_without_interrupt(500);
				whileswitch2=0;
				B=B-1;
				send_7seg(SEG_gpio, DIN_pin, CS_pin,  CLK_pin, 3, 10);
				send_7seg(SEG_gpio, DIN_pin, CS_pin,  CLK_pin, 1, B);
			//	display_number(SEG_gpio, DIN_pin, CS_pin, CLK_pin,B,num_digits(B));
			}
		}
	}//whileswitch
	whileswitch=1;
	goto restart;
	}
	}//main




