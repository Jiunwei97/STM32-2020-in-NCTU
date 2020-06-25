#include "stm32l476xx.h"
#include "helper_functions.h"
#include "7seg.h"
#include "keypad.h"
#include "led_button.h"
#include "timer.h"

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

// Define pins for button (default use on-board button PC13)
#define BUTTON_gpio GPIOC
#define BUTTON_pin 13

// Define Counter timer
#define COUNTER_timer TIM2

#define lab_keypad_single_key

const int threekeypad[3][3]={
						{1,2,3},
						{4,5,6},
						{7,8,9}
};

int main(){
	FPU_init();
	if(init_keypad(ROW_gpio, COL_gpio, ROW_pin, COL_pin) != 0){
			// Fail to init keypad
			return -1;
		}

	init_button(BUTTON_gpio, BUTTON_pin);
	if(init_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin) != 0){
			// Fail to init 7seg
			return -1;
		}

	GPIO_init_AF();
	//Enable timer
	timer_enable(TIM2);
	//Init the timer
	timer_init(TIM2,1, 20); //4M/2/21
	PWM_channel_init();
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
	//delay_without_interrupt(1000);
	while(1){
	int input=0;
	int save=0;
	int B =0;
	while(1){
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
					if(check_keypad_input_one(ROW_gpio, COL_gpio, ROW_pin, COL_pin, i, j)){
						input = 1;
						save = threekeypad[i][j];
						if (save>10){
							save=save%10;
						}
						display_number(SEG_gpio, DIN_pin, CS_pin, CLK_pin,save,num_digits(save));
						//delay_without_interrupt(400);
						break;
					}
				}
				}
	if(read_gpio(BUTTON_gpio, BUTTON_pin)==0)
		break;
	}
	double TIME_SEC=save;
		if(1){
				//send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i, 0);
				int num = (int)TIME_SEC;
				int num_dot = (int)(TIME_SEC*100);
				int num_d = num_digits(num);
				for(int i = 0 ; i< 2;i++){
						send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i+1, num_dot%10);
						num_dot/=10;
				}

				for(int i = 0 ; i< num_d ;i++){
							if(i==0)
								send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, 3,0x80 + num%10 );
							else
							send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i+3, num%10);
							num/=10;
				}

			}
			for (int i=1;i<=8;i++){
				send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i, 15);
			}
			timer_enable(COUNTER_timer);
			timer_init(COUNTER_timer,40000,100);
			timer_start(COUNTER_timer);
			int sec=-1;
			int last=0;
			last=COUNTER_timer;
			while(1){
				if(last!=COUNTER_timer->CNT){
					if(COUNTER_timer->CNT==0){
						sec++;
					}
					last =COUNTER_timer->CNT;
					double now_time = (double)sec + (double)(COUNTER_timer->CNT)/100;
					//display_two_decimal(SEG_gpio, DIN_pin, CS_pin, CLK_pin,now_time);
					//    print float
					int num = (int)now_time;
					int num_dot = (int)(now_time*100);
					int num_d = num_digits(num);
					for(int i = 0 ; i< 2;i++){
							send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i+1, num_dot%10);
							num_dot/=10;
					}

					for(int i = 0 ; i< num_d ;i++){
								if(i==0)
									send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, 3,0x80 + num%10 );
								else
								send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i+3, num%10);
								num/=10;
					}
					//
					if(now_time==TIME_SEC){
						timer_enable(TIM2);
						input = 1;
						timer_init(TIM2, 200000/784-1, 19); //4M/20/(200000/f)=f
						break;
					}
				}
			}

			timer_init(TIM2,40,80);
			timer_start(COUNTER_timer);
			int state=0;
			while(read_gpio(BUTTON_gpio, BUTTON_pin)==1){
						if(last!=COUNTER_timer->CNT){
								if(COUNTER_timer->CNT==0){
									if(state==0){
									}
									if(state==1){
									}
									state=!state;
								}
							}
						last=COUNTER_timer->CNT;
				}
			timer_stop(COUNTER_timer);
			for (int i=1;i<=8;i++){
						send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i, 15);
					}
		}

		return 0;
	}

