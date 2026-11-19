#include <STC32G.H>
#include "intrins.h" 
#include "delay.h"  
#include "uart1.h" 
#include "adc.h" 
/********************* 主函数 *************************/
void main4()
{ 
	
  WTST = 0;  //设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快
  EAXFR = 1; //扩展寄存器(XFR)访问使能
  CKCON = 0; //提高访问XRAM速度
 
	Uart1_Init(); //串口初始化 
	
	ADC_Init_Port(); //ADC初始化
	
  PrintString1("ADC Test!\r\n");  //UART1发送一个字符串 
	
  while (1)
	{
		User_Get_ADC_data(); //获取ADC
	} 
} 

