#include <STC32G.H>
#include "intrins.h"
#include "delay.h"  
#include "BEEP.h"   

/********************* 主函数 *************************/
void main9(void)
{	
	
 	WTST = 0;  //设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快
	EAXFR = 1; //扩展寄存器(XFR)访问使能
	CKCON = 0; //提高访问XRAM速度

	P3M0 = 0x40;         //将P3.6口单独设置为推挽输出模式  0100 0000
	P3M1 = 0x00;	
	
	Timer0Init(); //定时器初始化
	
  while (1)
	{ 
		
   Play_Music_Happy_Brithday(); //播放音乐
		
	}
}


