#include "Bluetooth.h" 
#include "motor.h" 
uint8 Car_State_number = 0; //小车状态值 ,蓝牙发送数据获取

//========================================================================
// 函数: void Calculation_Data(void)
// 描述: 根据状态数值去进行PWM调节
// 参数:
// 返回: Count_State_number  1  前进   
// 返回: Count_State_number  2  后退   
// 返回: Count_State_number  3  左转   
// 返回: Count_State_number  4  右转  
// 返回: Count_State_number  5  停止 
// 版本: VER1.0
// 日期: 2022-9-24
// 备注: 
//========================================================================  
void Adjust_Pwm_data(void)
{
		switch(Car_State_number)
		{
			case 1:  Car_Forword(PWM_Count);break;
			case 2:  Car_Back(PWM_Count);break;
			case 3:  Car_Turn_Left(PWM_Count);break;
			case 4:  Car_Turn_Right(PWM_Count);break;
			case 5:  Car_Stop();break;
			default :Car_Stop(); break;
		}	
}





























