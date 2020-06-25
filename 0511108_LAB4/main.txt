#include "stm32l476xx.h"
#include "7seg.h"
#include "keypad.h"
#include "helper_functions.h"

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

int main(){
	if(init_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin)!=0)
			return -1;

	send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,SEG_ADDRESS_DECODE_MODE,0xFF);
	send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,SEG_ADDRESS_SCAN_LIMIT,0x07);
	send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,SEG_ADDRESS_SHUTDOWN,0x01);
	send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,SEG_ADDRESS_ITENSITY,0x05);

	display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,0,0);

	if(init_keypad(ROW_gpio,COL_gpio,ROW_pin,COL_pin)!=0)
		return -1;

	int B=0;//暫存器B，存取運算值
	double equ[1000];
	int k=0;
	//錯誤偵測
	int while_switch=1;
	int check_state=0;
	int check=0;
	restart:
	while(while_switch){
		for(int i=0;i<4;i++){
			for(int j=0;j<4;j++){
				if(check_keypad_input_one(ROW_gpio,COL_gpio,ROW_pin,COL_pin,i,j)){
					int A = keypad[i][j];//暫存器A
					delay_without_interrupt(100);

					if(A<10){
						B = B*10+A;
						if(B>999)
							B/=10;
						display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,B,num_digits(B));
						check_state=1;
					}
					else if(A<14){
						if(check_state==1){
							check+=1;
							check_state=0;
						}

						equ[k]=B;
						display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,0,0);
						B=0;
						equ[k+1]=A;
						k += 2;
						check-=1;
					}
					else if(A==15){
						equ[k]=B;
						B=0;

						display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,0,0);
						delay_without_interrupt(200);

						if(check==0)
							while_switch=0;
						else{
							send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,3,SEG_DATA_DECODE_DASH);
							send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,2,1);
						}
					}
					else{
						B=0;
						k=0;
						check=0;
						display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,0,0);
					}
				}
			}
		}
	}

	//DIV
	for(int j=1;j<=k-1;j+=2){
		if(equ[j]==13){
			equ[j-1] = equ[j-1]/equ[j+1];
			for(int m=j+2;m<=k;m++)//後方未計算的元素往前搬動整理，從j+2到k的元素都要被搬動
				equ[m-2]=equ[m];
			k -= 2;
			j -= 2;
		}
	}

	//MULP
	for(int j=1;j<=k-1;j+=2){
		if(equ[j]==12){
			equ[j-1] = equ[j-1]*equ[j+1];
			for(int m=j+2;m<=k;m++)
				equ[m-2]=equ[m];
			k -= 2;
			j -= 2;
		}
	}

	//SUM
	for(int j=1;j<=k-1;j+=2){
		if(equ[j]==10){
			equ[j-1] = equ[j-1]+equ[j+1];
			for(int m=j+2;m<=k;m++)
				equ[m-2]=equ[m];
			k -= 2;
			j -= 2;
		}
	}

	//SUB
	for(int j=1;j<=k-1;j+=2){
		if(equ[j]==11){
			equ[j-1] = equ[j-1]-equ[j+1];
			for(int m=j+2;m<=k;m++)
				equ[m-2]=equ[m];
			k -= 2;
			j -= 2;
		}
	}


	int ANS= equ[0]*1000;
	int DP=4;
	for(int i=1;i<=3;i++){
		if(ANS%10==0){
			ANS/=10;
			DP--;
		}
	}

	int num_with_DP=ANS;
	for(int i=1;i<DP;i++){
		num_with_DP/=10;
	}
	num_with_DP%=10;

	if(ANS<0){
		ANS *= -1;
		num_with_DP *= -1;
		display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,ANS,num_digits(ANS));
		send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,DP,num_with_DP|0b10000000);
		send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,num_digits(ANS)+2,SEG_DATA_DECODE_DASH);
	}
	else{
		display_number(SEG_gpio,DIN_pin,CS_pin,CLK_pin,ANS,num_digits(ANS));
		send_7seg(SEG_gpio,DIN_pin,CS_pin,CLK_pin,DP,num_with_DP|0b10000000);
	}

	while_switch=1;
	goto restart;
}
