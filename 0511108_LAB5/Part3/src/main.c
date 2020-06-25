#include "stm32l476xx.h"
#include "7seg.h"
#include "keypad.h"
#include "helper_functions.h"
#include "led_button.h"
#include "timer.h"

// Define pins for 7seg
#define SEG_gpio GPIOC
#define DIN_pin 3
#define CS_pin 4
#define CLK_pin 5
// Define pins for keypad
#define COL_gpio GPIOA
#define COL_pin 5       // 5 6 7 8
#define ROW_gpio GPIOB
#define ROW_pin 3       // 3 4 5 6

//Define pins for led(default use on-board led PA5)
#define LED_gpio GPIOA
#define LED_pin 5

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

int read_gpio(GPIO_TypeDef* gpio, int pin)
{
	if((gpio->IDR)>>pin & 1)
		return 1; //no touch ‚÷1
	else
		return 0; //touch ‚÷0
}

int init_7seg(GPIO_TypeDef* gpio, int DIN, int CS, int CLK)
{
	//Enable AHB2 Clock
	if(gpio == GPIOA)
	{
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	}
	else if(gpio == GPIOB)
	{
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	}
	else if(gpio == GPIOC)
	{
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
	}
	else
	{
		//Error! Add other cases to suit other GPIO pins
		return -1;
	}
	//Set GPIO pins to output mode (01)
	//First Clear bits(&) then set bits (|)
	gpio->MODER &= ~(0b11 << (2*DIN));
	gpio->MODER |= (0b01 << (2*DIN));
	gpio->MODER &= ~(0b11 << (2*CS));
	gpio->MODER |= (0b01 << (2*CS));
	gpio->MODER &= ~(0b11 << (2*CLK));
	gpio->MODER |= (0b01 << (2*CLK));

	//Close display test
	send_7seg(gpio, DIN, CS, CLK, SEG_ADDRESS_DISPLAY_TEST, 0x00);
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
int display_number(GPIO_TypeDef* gpio, int DIN, int CS, int CLK, int num, int num_digs)
{
	for(int i = 1;i<=num_digs;i++)
	{
		send_7seg(gpio, DIN, CS, CLK, i, num % 10);
		num /= 10;
	}
	for(int i = num_digs+1;i<=8;i++)
	{
		num/=10;
		send_7seg(gpio, DIN, CS, CLK, i, 15);//blank
	}
	if(num!=0)
		return -1;
	return 0;
}
/*
void delay_without_interrupt(int msec){
	int loop_cnt=500*msec;

	while(loop_cnt)
		loop_cnt --;
	return;
}
*/
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

void GPIO_init_AF(){
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	//Set to Alternate function mode
	GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk;
	GPIOA->MODER |= (2 << GPIO_MODER_MODE0_Pos);
	//Set AFRL
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0_Msk;
	GPIOA->AFR[0] |= (1 << GPIO_AFRL_AFSEL0_Pos);
}


void timer_enable(TIM_TypeDef *timer){
	if(timer == TIM2){
		RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	}
	else if(timer == TIM3){
		RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
	}
}

void timer_disable(TIM_TypeDef *timer){
	if(timer == TIM2){
		RCC->APB1ENR1 &= ~RCC_APB1ENR1_TIM2EN;
	}
	else if(timer == TIM3){
		RCC->APB1ENR1 &= ~RCC_APB1ENR1_TIM3EN;
	}
}


void timer_init(TIM_TypeDef *timer, int psc, int arr){
	//fout =
	timer->PSC = (uint32_t)(psc-1);  //PreScalser
	timer->ARR = (uint32_t)(arr-1);  //Reload value
	timer->EGR |= TIM_EGR_UG;        //Reinitialze the counter

}

void timer_start(TIM_TypeDef *timer){
	timer->CR1 |= TIM_CR1_CEN;
}


void PWM_channel_init(){
	//p.883 915 920 924
	//PA0 for PWM
	//PWM mode 1
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk;
	TIM2->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos);
	//OCPreload_Enable
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1PE_Msk;
	TIM2->CCMR1 |= (1 << TIM_CCMR1_OC1PE_Pos);
	//Active high for channel 1 polarity
	TIM2->CCER &= ~TIM_CCER_CC1P_Msk;
	TIM2->CCER |= (0 << TIM_CCER_CC1P_Pos);
	//Enable for channel 1 output
	TIM2->CCER &= ~TIM_CCER_CC1E_Msk;
	TIM2->CCER |= (1 << TIM_CCER_CC1E_Pos);
	//Set Compare Register
	TIM2->CCR1 = 10;
	//Set PreScaler
	TIM2->PSC = 0;
}

int init_keypad(GPIO_TypeDef* row_gpio, GPIO_TypeDef* col_gpio, int row_pin, int col_pin){
	//Enable AHB2 Clock
	if(row_gpio == GPIOA || col_gpio == GPIOA){
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	}
	if(row_gpio == GPIOB || col_gpio == GPIOB){
		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	}
	//First Clear bits(&) then set bits (|)
	for(int a=0; a<4; a++)
	{
		//Set GPIO pins to output mode (01)
		col_gpio -> MODER &= ~(0b11 << (2*(col_pin+a)));
		col_gpio -> MODER |= (0b01 << (2*(col_pin+a)));
		//Set GPIO pins to very high speed mode (11)
		col_gpio -> OSPEEDR &= ~(0b11 << (2*(col_pin+a)));
		col_gpio -> OSPEEDR |= (0b11 << (2*(col_pin+a)));
		//Set GPIO pins to open drain mode (1)
		col_gpio -> OTYPER &= ~(0b1 << (col_pin+a));
		col_gpio -> OTYPER |= (0b1 << (col_pin+a));
		//Set Output to high
		set_gpio(col_gpio, col_pin+a);
	}
	//First Clear bits(&) then set bits(|)
	for(int a=0; a<4; a++)
	{
		//Set GPIO pins to input mode (00)
		row_gpio -> MODER &= ~(0b11 << (2*(row_pin+a)));
		row_gpio -> MODER |= (0b00 << (2*(row_pin+a)));
		//Set GPIO pins to Pull-Down mode (10)
		row_gpio -> PUPDR &= ~(0b11 << (2*(row_pin+a)));
		row_gpio -> PUPDR |= (0b10 << (2*(row_pin+a)));
	}
	return 0;
}

int check_keypad_input_one(GPIO_TypeDef* row_gpio, GPIO_TypeDef* col_gpio, int row_pin, int col_pin, int x, int y)
{
	int cycles = 400;
	//Set Column to push-pull mode
	col_gpio -> OTYPER &= ~(1 << (col_pin+y));
	//Count the total number of time it is pressed in a certain period
	int cnt = 0;
	for(int a=0; a<cycles; a++){
		cnt += read_gpio(row_gpio, row_pin+x);
	}
	//Set Column back to open drain mode
	col_gpio -> OTYPER |= (1 << (col_pin+y));
	//return if the key is pressed(1) or not(0)
	return (cnt > (cycles*0.7));
}


const int three[3][3]={
				{523, 587, 659},
				{698, 784,880},
				{988 ,1046,0}

};

void timer_stop(TIM_TypeDef *timer){
	timer->CR1 &= ~TIM_CR1_CEN;
}

const int keypad[4][4] = {
	{1, 2, 3, 10},
	{4, 5, 6, 11},
	{7, 8, 9, 12},
	{15, 0, 14, 13}
};

int main(){
	FPU_init();
	if(init_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin) != 0)
	{
		//Fail to init 7seg
		return -1;
	}

	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_DECODE_MODE, 0xFF);

	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SCAN_LIMIT, 0x07);

	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_SHUTDOWN, 0x01);

	send_7seg(SEG_gpio, DIN_pin, CS_pin, CLK_pin, SEG_ADDRESS_ITENSITY, 0x05);

	/*
	if(init_button(BUTTON_gpio, BUTTON_pin)!=0){
		//Failed to init button
		return -1;
	}
	*/

	if(init_keypad(ROW_gpio,COL_gpio,ROW_pin,COL_pin)!=0){
		return -1;
	}

	GPIO_init_AF();
	//Enable timer
	timer_enable(TIM2);
	//Init the timer
	timer_init(TIM2,1, 20); //4M/2/21
	PWM_channel_init();
	timer_start(TIM2);

	int octave = 3;
	int f;
	while(1){
		display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,0,0);
		int input = 0;
		for(int i=0;i<4;i++)
		{
			for(int j=0;j<4;j++)
			{
				if(check_keypad_input_one(ROW_gpio, COL_gpio, ROW_pin, COL_pin, i, j))
				{
					display_number(SEG_gpio, DIN_pin, CS_pin, CLK_pin,keypad[i][j],2);
						if(octave == 3){
							f = three[i][j];
						}
						timer_enable(TIM2);
						input = 1;
						timer_init(TIM2, 200000/f-1, 19); //4M/20/(200000/f)=f
					}
				}

		}
		if(input == 0)
		{
			timer_disable(TIM2);
		
		}
	}

}
