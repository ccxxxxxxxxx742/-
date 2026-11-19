#include <STC32G.H>
#include "intrins.h"
#include "delay.h"  
#include "motor.h" 
#include "timer.h"    
#include "LED.h"  
#include "BEEP.h"  
/********************* 引脚定义 *************************/ 

sbit KEY=P3^7;		//重定义按键引脚功能 

unsigned char model_Number=1 ; 

/********************* 主函数 *************************/
void main()
{
	
   WTST = 0;  //设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快
   EAXFR = 1; //扩展寄存器(XFR)访问使能
   CKCON = 0; //提高访问XRAM速度
  
	 LED_Init_Port(); //LED端口输出
	
	 Motor_Init_Port(); //电机初始化
	
	 BEEP_Init_Port(); //蜂鸣器初始化
	
	 Timer0Init(); //定时器0，蜂鸣器使用
	 
	 Timer4Init(); //定时器初始化,计数时间使用
	
		while (1)
		{   
/*********************************按键检测*******************************************************/				
			  if(KEY==0)
				{
					delay_ms(20);   //20ms防抖检测
					if(KEY==0)			//再次检测按键情况
					{ 
						if(++model_Number>2)
						{
							 model_Number=0;
						}	
					}
					while(KEY==0); 	//等待松开
				} 
		
/*********************************汽车前进*******************************************************/ 
         if(model_Number==1)
				 {
					 Car_State_number=1;   
					 
				 }else if(model_Number==2) 
				 { 
					 Car_State_number=5;          //  汽车停下  
					 delay_ms(2000); 
					 Car_State_number=1;          //  汽车前进  
					 delay_ms(800); 
					 Car_State_number=5;          //  汽车停下  
					 
		       LED_ON();  //LED亮 
			     delay_ms(200); 
			     LED_OFF(); //LED灭 
			     delay_ms(200);  
			
		       LED_ON();  //LED亮 
			     delay_ms(200); 
			     LED_OFF(); //LED灭 
			     delay_ms(200);  
						
		       LED_ON();  //LED亮 
			     delay_ms(200); 
			     LED_OFF(); //LED灭 
			     delay_ms(200); 
			
		       LED_ON();  //LED亮 
			     delay_ms(200); 
			     LED_OFF(); //LED灭 
			     delay_ms(200);  		

		       LED_ON();  //LED亮 
			     delay_ms(200); 
			     LED_OFF(); //LED灭 
			     delay_ms(200);  
					 
					 Play_Music_Happy_Brithday();//  播放声音  
					 
					 Car_State_number=2;          //  汽车后退  
					 delay_ms(800); 
 					 Car_State_number=5;          //  汽车停下  
						
					 model_Number=0;
				 }
		}
}























