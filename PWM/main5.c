#include <STC32G.H>
#include "intrins.h"
#include "timer.h" 
#include "delay.h"  
#include "motor.h" 

sbit KEY=P3^7;		//重定义按键引脚功能

/********************* 主函数 *************************/
void main5()
{
	
  WTST = 0;  //设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快
  EAXFR = 1; //扩展寄存器(XFR)访问使能
  CKCON = 0; //提高访问XRAM速度 
	
	P3M0 = 0x00;        
  P3M1 = 0x80;        //将P3.7口设为高阻输入模式        1000 0000
  P3PU = 0x80;        //将P3.7引脚内部上拉电阻打开        1000 0000
	
	Motor_Init_Port();  //电机初始化
	
	Timer0Init();    //定时器初始化

  while (1)
	{
		if(KEY==0)
		 {
				 delay_ms(20);   //20ms防抖检测
				if(KEY==0)			//再次检测按键情况
				{
				  if(PWM_Count<100) //按键调速 100--100%
					{
						PWM_Count+=20;
					}else{						
						PWM_Count=5;
					}
				}while(KEY==0); 	//当按键没有放开时，不跳出循环
		}
	}
}

