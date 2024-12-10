#ifndef BOOTLOADER_H
#define BOOTLOADER_H

// 引入STM32F4xx的硬件抽象层头文件，根据具体STM32型号可能需要替换xxx
#include "stm32f4xx_hal.h"  
#include <string.h>
#include <stdint.h>
#include <stdio.h>

//--------------------------根据实际的芯片修改----------------------------------
// Bootloader标志位所在的内存地址，用于存储一些标志信息来控制Bootloader的流程等
#define BOOTLOADER_FLAG_ADDRESS  0x08008000  
// 第一个用户应用程序的起始地址，用于跳转到该应用程序
#define APP1_FLAG_ADDRESS 0x08020000
// 第二个用户应用程序的起始地址，用于跳转到该应用程序
#define APP2_FLAG_ADDRESS 0x08040000

// 函数原型声明，以下函数在bootloader.c文件中实现，此处声明以便其他文件调用


//--------------------------不需要修改的代码----------------------------------
// Bootloader主流程函数，负责检查标志并决定跳转等操作
void Bootloader_Run(void);
// 用于从Bootloader跳转到指定地址的应用程序的函数
void JumpToApplication(uint32_t APPLICATION_ADDRESS);
// 错误处理函数,main.c中的代码
void Error_Handler(void);
// 修改Bootloader标志位的函数，用于更新相关标志值
int Update_Bootloader_Flag(uint32_t Sector,uint32_t new_flag);

//--------------------------可能需要修改的代码----------------------------------
// Bootloader初始化函数，可用于初始化相关硬件资源等
void Bootloader_Init(void);
// 用于擦除应用程序所在Flash扇区的函数，参数为要擦除的扇区编号
void APP_Flash_Erase(uint32_t Sector);
// 用于向Flash中写入应用程序固件数据的函数，参数包括写入起始地址、固件数据缓冲区指针和固件数据大小
void APP_Flash_Write(uint32_t APP_FLAG_ADDRESS,uint8_t *firmware_buffer,uint32_t firmware_size);
// 用于更新应用程序固件的函数，整合了擦除、写入以及更新标志等操作，参数与APP_Flash_Write类似
void APP_Firmware_Updata(uint32_t APP_FLAG_ADDRESS,uint8_t *firmware_buffer,uint32_t firmware_size);

#endif /* BOOTLOADER_H */
