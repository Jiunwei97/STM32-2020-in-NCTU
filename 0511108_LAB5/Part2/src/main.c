#include "stm32l476xx.h"
#include "7seg.h"
#include "keypad.h"
#include "helper_functions.h"
#include "led_button.h"
#include "timer.h"

//Define pins for 7seg
#define SEG_gpio GPIOC
#define DIN_pin 3
#define CS_pin 4
#define CLK_pin 5

//Define pins for keypad
#define COL_gpio GPIOA
#define COL_pin 6	//6 7 8 9
#define ROW_gpio GPIOB
#define ROW_pin 3	//3 4 5 6

//Define pins for led(default use on-board led PA5)
#define LED_gpio GPIOA
#define LED_pin 9

//Define pins for button (default use  on-board button PC13)
#define BUTTON_gpio GPIOC
#define BUTTON_pin 13

//Define Counter timer
#define COUNTER_timer TIM2

void set_gpio(GPIO_TypeDef* gpio, int pin){
		gpio->BSRR |=( 1<< pin);
}

void reset_gpio(GPIO_TypeDef* gpio, int pin){
		gpio->BRR |=( 1<< pin);
}

int init_7seg(GPIO_TypeDef* gpio,int DIN,int CS,int CLK){
	//Enable AHB2 Clock
	if(gpio == GPIOA)
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	else if(gpio == GPIOB)
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	else if(gpio ==GPIOC)
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
	else
		//Error! Add other cases to suit other GPIO pins
		return -1;

	//將腳位設為OUTPUT
	gpio -> MODER &= ~(0b11 << (2*DIN));
	gpio -> MODER |= (0b01 << (2*DIN));
	gpio -> MODER &= ~(0b11 << (2*CS));
	gpio -> MODER |= (0b01 << (2*CS));
	gpio -> MODER &= ~(0b11 << (2*CLK));
	gpio -> MODER |= (0b01 << (2*CLK));

	//關閉DISPLAY TEST
	send_7seg(gpio,DIN,CS,CLK,SEG_ADDRESS_DISPLAY_TEST,0x00);

	return 0;
}

void send_7seg(GPIO_TypeDef* gpio, int DIN, int CS, int CLK, int address, int data)
{
	//The payload to send
	int payload = ((address&0xFF)<<8)|(data&0xFF);

	//Start the sending cycles
	//16 data-bits + 1 CS signal
	int total_cycles = 16+1;

	for(int a=1;a<=total_cycles;a++)
	{
		//Reset CLK when enter
		reset_gpio(gpio,CLK);

		//Set DIN according to data except for last cycles(CS)
		if(((payload>>(total_cycles-1-a))&0x1) && a!= total_cycles)
		{
			set_gpio(gpio, DIN);
		}
		else
		{
			reset_gpio(gpio, DIN);
		}
		 //Set CS at last cycle
		if(a == total_cycles)
		{
			set_gpio(gpio, CS);
		}
		else
		{
			reset_gpio(gpio, CS);
		}

		//Set CLK when leaving (seg set data at rising edge)
		set_gpio(gpio, CLK);
	}

	return;
}

int display_two_decimal(GPIO_TypeDef* gpio, int DIN, int CS, int CLK, double num){
	//Set two decimal points
	int dec_1=(int)(num*10)%10, dec_2=(int)(num*100)%10;
	send_7seg(gpio, DIN, CS, CLK, 2, dec_1);
	send_7seg(gpio, DIN, CS, CLK, 1, dec_2);

	int number =num;
	//Set third digit, add a decimal dot
	send_7seg(gpio, DIN, CS, CLK, 3, 0x80+number%10);
	number = number/10; //test this line, still no understand it <------------

	for(int i=4; i<=8; i++){
		if(number>0){
			send_7seg(gpio, DIN, CS, CLK, i, number%10);
			number = number/10;
		}
		else{
			send_7seg(gpio, DIN, CS, CLK, i, 15);
		}
	}

	return 0;
}

void timer_enable(TIM_TypeDef *timer){
	if(timer == TIM2){
		RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	}
	else if(timer == TIM3){
		RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
	}
}

void timer_init(TIM_TypeDef *timer, int psc, int arr){
	timer->PSC = (uint32_t)(psc-1);  //PreScalser
	timer->ARR = (uint32_t)(arr-1);  //Reload value
	timer->EGR |= TIM_EGR_UG;        //Reinitialze the counter

}

void timer_start(TIM_TypeDef *timer){
	timer->CR1 |= TIM_CR1_CEN;
}

void timer_stop(TIM_TypeDef *timer){
	timer->CR1 &= ~TIM_CR1_CEN;
}


void FPU_init(){
	//Setup FPU
	SCB -> CPACR |= (0xF << 20);
	__DSB();
	__ISB();
}


int init_button(GPIO_TypeDef* gpio, int button_pin)
{
	//Enable AHB2 Clock
	if(gpio==GPIOC)
	{
		RCC->AHB2ENR |=RCC_AHB2ENR_GPIOCEN;
	}
	else
	{
		//Error! Add other cases to suit other GPIO pins
		return -1;
	}
	//Set GPIO pins to input mode (00)
	//First Clear bits(&) then set bits (|)
	gpio->MODER &= ~(0b11<<(2*button_pin));
	gpio->MODER |= (0b00<<(2*button_pin));

	return 0;
}

int main(){
	FPU_init();
	if(init_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin) != 0)
	{
		//Fail to init 7seg
		return -1;
	}
	//Set Decode Mode to Code B decode mode
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_DECODE_MODE, 0xFF);
	//Set Scan Limit to all digits
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SCAN_LIMIT, 0x07);
	//Wakeup 7seg
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SHUTDOWN, 0x01);
	//Set brightness
	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_ITENSITY, 0x05);
	//Clear the digits
	for(int i=1;i<=8;i++){
		send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, i, 15);
	}

	double TIME_SEC = 12.34;
	//Check time bound
	if(TIME_SEC < 0.0 || TIME_SEC > 10000.0){
		display_two_decimal(SEG_gpio, DIN_pin, CS_pin, CLK_pin, 0.0);
	}
	else{
		//Enable time
		timer_enable(COUNTER_timer);
		//Init the timer
		timer_init(COUNTER_timer, 40000, 100); //ARR PSC fout=4M/40000+1/100+1 每1/100秒CNT+1，CNT到100時歸零
		//Start the timer
		timer_start(COUNTER_timer);

		int sec=0, last = 0;
		while(1){
			if(last!=COUNTER_timer -> CNT){//100是CNT上限
				if(COUNTER_timer -> CNT ==0){
					//One second has passed
					sec++;
				}
				last = COUNTER_timer -> CNT;//保持此迴圈跟CNT等速度
				double now_time = sec+COUNTER_timer -> CNT/100.0;
				display_two_decimal(SEG_gpio, DIN_pin, CS_pin, CLK_pin, now_time);
				if(now_time == TIME_SEC){
              	timer_init(TIM2, 200000/784-1, 19); //4M/20/(200000/f)=f            
					break;
				}
			}
		}
		timer_stop(COUNTER_timer);
	}
}
