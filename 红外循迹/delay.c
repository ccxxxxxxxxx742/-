#include "delay_2.h" 
#include "intrins.h"
#define MAIN_Fosc        24000000UL   //定义主时钟 

//========================================================================
// 函数: void delay_ms(u8 ms)
// 描述: 延时函数。
// 参数: ms,要延时的ms数 自动适应主时钟.
// 返回: none.
// 版本: VER1.0
// 日期: 2022-9-24
// 备注: 
//========================================================================
void delay_ms(unsigned char ms)
{
    unsigned int  i;
    do{
        i = MAIN_Fosc / 6000;
        while(--i);   //6T per loop
      }while(--ms);
}
void Delay100ms(void)		//@24.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 13;
	j = 45;
	k = 214;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}






