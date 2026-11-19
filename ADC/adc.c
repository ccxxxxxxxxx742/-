#include "adc.h"  
#include "uart1.h" 
#include "delay.h" 
#include "stdio.h" 
#include "intrins.h" 
#include <stdio.h>
int BGV,BGP,VCC;

 /**************************************************************************
功能描述：ADC口初始化
入口参数：无
返回值：无
 *************************************************************************/
void	ADC_config(void)
{	
	P_SW2 |= 0x80;		      //将EAXFR位置1，以访问在XDATA区域的扩展SFR
	ADCTIM = 0x3f;		      //设置ADC内部时序，ADC采样时间建议设最大值
	P_SW2 &= 0x7F;		      //将EAXFR位置0，恢复访问XRAM	

	ADCCFG|=0x20;           //ADC转换结果ADC_RES存高4位，ADC_RESL存低8位
	ADCCFG|=0x0F;           //设置ADC时钟为系统时钟/2/16/16

	ADC_CONTR&=0xDF;        //清AD转换完成标志
	ADC_CONTR|=0x80;      	//开AD转换电源
	
}
/**************************************
功能描述：ADC口检测AD转换值函数
入口参数：无
返回值：ADC 12位数据
***************************************/
uint16	Get_ADC12bitResult(uint8 channel)	
{
	  ADC_RES = 0;        //将ADC转换结果高位寄存器ADC_RES清空
    ADC_RESL = 0;       //将ADC转换结果低位寄存器ADC_RESL清空

    ADC_CONTR = (ADC_CONTR & 0xF0) | 0x40 | channel;  //启动AD转换并选择通道
    _nop_();
    _nop_();
    _nop_();
    _nop_();	
	  while((ADC_CONTR & 0x20) == 0) ;   //等待AD转换完成
    ADC_CONTR &=0xDF;                    //清AD转换完成标志
    return  (((uint16)ADC_RES << 8) | ADC_RESL);
	
}/**************************************
功能描述：初始化要获取的ADC端口
入口参数：无              P01------------------------ADC9
返回值：ADC 12位数据      读内部基准电压             ADC15           - 
***************************************/
void ADC_Init_Port(void)
{                                              
	
  P0M0 = 0x00;	                  //设置P0.1为高阻输入  0000 0010 
	P0M1 = 0x02;	
	
	
	ADC_config();
}
 /**************************************************************************
功能描述：获取ADC值，并且打印出来  0000 0111  0000 0111
入口参数：无
返回值：无 1236   // 0000 0111
 *************************************************************************/ 

void User_Get_ADC_data(void)
{  
	   uint16 ADCdata=0 ; 
	
	   float g_voltage=0;	
	
     ADCdata = Get_ADC12bitResult(9);     //读外部电压ADC   
	
	   //2V5为ADC基准电压,所以计算电压值公式：V=Temp_signal/4096*2.5 //	  
    
	   if(ADCdata>=4096)//超量程
		 {
			 ADCdata=4096;
		 }
	 
	   g_voltage=((2.5*ADCdata)/4096)*1000;	 //扩大1000倍，mv打印
	   
	   g_voltage=g_voltage*3; //电阻分压了
	
		 Uart_Send_ADC_data(g_voltage); 
	
	   delay_ms(200); 
}	 
 /**************************************************************************
功能描述：串口打印数据
入口参数：无
返回值：无 1236
 *************************************************************************/ 
void Uart_Send_ADC_data(uint16 ADC_DATA)
{  
	  unsigned char temp=0,temp1=0,temp2=0,temp3=0;
	
	     temp=(ADC_DATA/1000)+'0'; 
	     temp1=(ADC_DATA/100%10)+'0'; 
	     temp2=(ADC_DATA/10%10)+'0';
	     temp3=(ADC_DATA%10)+'0';
	
	  	 PrintString1("ADC :"); //
			 SendDataByUart1(temp);
			 SendDataByUart1(temp1 );
			 SendDataByUart1(temp2);
			 SendDataByUart1(temp3); 		
		   PrintString1(" mV\r\n");		
}
/*********************************END FILE********************************************/	





















