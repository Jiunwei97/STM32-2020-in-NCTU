#include "stm32l476xx.h"
#include "7seg.h"
#include "keypad.h"
#include "helper_functions.h"
#include "led_button.h"
#include "timer.h"

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


void FPU_init(){
	//Setup FPU
	SCB -> CPACR |= (0xF << 20);
	__DSB();
	__ISB();
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

void delay_without_interrupt(float msec){
	int loop_cnt = 500*msec;
	while(loop_cnt){
		loop_cnt--;
	}
	return;
}

void PWM_channel_init(int ccr){
	//p.883 915 920 924
	//PA0 for PWM
	//PWM mode 1  //if cnt <ccr then set high
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
	TIM2->CCR1 = ccr ; //duty cycle = ccr1/(arr+1)
	//Set PreScaler
	TIM2->PSC = 799;
	TIM2->ARR = 99;
}


void timer_stop(TIM_TypeDef *timer){
	timer->CR1 &= ~TIM_CR1_CEN;
}

//a period of 20ms and a duty cycle between 1ms and 2ms is needed.

int main(){
	FPU_init();
	GPIO_init_AF();
	//Enable timer
	timer_enable(TIM2);
	//Init the timer
	timer_init(TIM2,799,99); //4M/2/21  <--- period

	timer_start(TIM2);
	while(1){
		PWM_channel_init(5);  //45 degree duty cycle =5% ; period=20 ms, ccr=5
		delay_without_interrupt(1000); //delay 1s
		PWM_channel_init(7.5); //90
		delay_without_interrupt(1000); //delay 1s
		PWM_channel_init(10); //135
		delay_without_interrupt(1000); //delay 1s
		PWM_channel_init(12.5);//180
		delay_without_interrupt(1000); //delay 1s
		PWM_channel_init(12.5);//180
		delay_without_interrupt(1000); //delay 1s
		PWM_channel_init(10); //135
		delay_without_interrupt(1000); //delay 1s
		PWM_channel_init(7.5); //90
		delay_without_interrupt(1000); //delay 1s
		PWM_channel_init(5);  //45 degree duty cycle =5% ; period=20 ms, ccr=5
		delay_without_interrupt(1000); //delay 1s

	}

		//	timer_disable(TIM2);

}


