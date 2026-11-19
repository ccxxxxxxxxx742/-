#include <STC32G.H>
#include "intrins.h"
#include "delay.h"  
#include "motor.h" 
#include "timer.h"    
#include "uart1.h" 
#include "sr04.h" 
/********************* 主函数 *************************/
void main6()
{
	
   WTST = 0;  //设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快
   EAXFR = 1; //扩展寄存器(XFR)访问使能
   CKCON = 0; //提高访问XRAM速度
	 
	 Motor_Init_Port(); //电机初始化
	
	 Timer0Init(); //定时器 初始化 ,超声波使用
	 
	 Timer4Init(); //定时器初始化,计数时间使用
	
	 Uart1_Init(); //串口初始化 
	 
	 SR04_Init_Port(); // 超声波IO初始化
	
  while (1)
	{ 

		  if(Time_Count>=10) //100ms更新一次超声波距离
			{				
				 Count_Number_DATA(); //测量距离
								
				 Time_Count=0; //重新计时
	    }  
			
		if(S>=20) //直走
		{
      Car_State_number=1; 
		}			
		else if(S<20&&S>=10)
					{ 
						Car_State_number=2;//后退
						delay_ms(100);
						Car_State_number=3; //左转 
						delay_ms(2000);
					}			
		else if(S<10&&S>=3)
					{
						 Car_State_number=2;//后退
						 delay_ms(500);
						 Car_State_number=4; //右3转  
						 delay_ms(500);
					}else{

           Car_State_number=2;//后退

					}						
				
  } 
}























