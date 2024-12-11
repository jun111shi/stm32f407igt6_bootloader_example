#include "bootloader.h"

// Bootloader初始化函数,可以在这里初始化一些硬件资源，例如用于指示故障的LED等，当前函数体暂为空实现
void Bootloader_Init(void)
{
    // 如果有必要，可以初始化 GPIO 或其他资源
    // 例如，用于故障指示的 LED 初始化
}

// 修改Bootloader标志位的函数，用于更新Bootloader相关的标志值
// 参数Sector是标志位使用的扇区号
// 参数new_flag是要更新成的新标志值
int Update_Bootloader_Flag(uint32_t Sector,uint32_t new_flag)
{
    HAL_StatusTypeDef status;
		
// 用于擦除应用程序所在Flash扇区的函数，参数为要擦除的扇区编号
		APP_Flash_Erase(Sector);

		// 解锁Flash
    status = HAL_FLASH_Unlock();
    if (status!= HAL_OK)
    {
        printf("Flash Unlock Error\r\n");
        return -1;
    }
	
    // 以字（32位）为单位将新的标志位写入Flash中指定的BOOTLOADER_FLAG_ADDRESS地址处
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, BOOTLOADER_FLAG_ADDRESS, new_flag);
    if (status!= HAL_OK)
    {
        printf("Flash Program Error\r\n");
        HAL_FLASH_Lock();
        return -1;
    }

    //锁定Flash，防止意外的Flash操作导致数据异常
    HAL_FLASH_Lock();

    // 验证写入是否成功，使用volatile关键字确保每次都从内存地址读取最新值，而不是使用缓存值
    uint32_t read_back = *(volatile uint32_t *)BOOTLOADER_FLAG_ADDRESS;
    if (read_back!= new_flag)
    {
        printf("Flash Verify Failed: read_back = %u\r\n", read_back);
        return -1;
    }

    printf("Bootloader Flag Updated to %u\r\n", new_flag);
    return 0;
}

// Bootloader主流程函数，负责检查Bootloader标志并决定后续操作
void Bootloader_Run(void)
{
    // 从BOOTLOADER_FLAG_ADDRESS地址处读取Bootloader标志值
    uint32_t bootloader_flag = *(volatile uint32_t*)BOOTLOADER_FLAG_ADDRESS;
    printf("bootloader_flag = %u\r\n",bootloader_flag);
    if (bootloader_flag == 0x01)
    {
        // 如果标志值为0x01，则跳转到用户应用程序，传入的地址为APP1_FLAG_ADDRESS
        JumpToApplication(APP1_FLAG_ADDRESS);
    }
    else if(bootloader_flag == 0x02){
        // 如果标志值为0x02，则跳转到另一个用户应用程序，传入的地址为APP2_FLAG_ADDRESS
        JumpToApplication(APP2_FLAG_ADDRESS);
    }
    else{
        // 如果标志值不是上述特定值，则先将Bootloader标志更新为0x01
        Update_Bootloader_Flag(2,0x01);
        // 然后跳转到用户应用程序，传入的地址为APP1_FLAG_ADDRESS
        JumpToApplication(APP1_FLAG_ADDRESS);
    }
}

// 跳转到应用程序的函数，实现从Bootloader跳转到指定地址的应用程序的功能
// 参数appAddress是要跳转到的应用程序的起始地址
void JumpToApplication(uint32_t appAddress)
{
    typedef void (*pFunction)(void);
    pFunction Jump_To_Application;

    // 关闭全局中断，防止在跳转过程中出现意外的中断干扰
    __disable_irq();

    // 复位向量表指向应用程序地址，将系统控制块（SCB）的向量表偏移寄存器（VTOR）设置为指定的应用程序地址
    SCB->VTOR = appAddress;

    // 获取应用程序复位处理函数地址，从应用程序向量表中的复位向量位置（地址 + 4 处）获取函数指针
    Jump_To_Application = (pFunction) * (__IO uint32_t *)(appAddress + 4);

    // 清除相关的系统控制块标志位等（可选操作，根据实际情况决定是否需要）
    __set_MSP(*(__IO uint32_t *)appAddress);
    // 跳转到应用程序复位处理函数，开始执行应用程序
    Jump_To_Application();
}

// 用于擦除应用程序所在Flash扇区的函数
// 参数Sector是要擦除的Flash扇区编号
void APP_Flash_Erase(uint32_t Sector)
{
    HAL_StatusTypeDef status;
    uint32_t pageError;

    FLASH_EraseInitTypeDef eraseInitStruct;

    // 设置擦除类型为按扇区擦除
    eraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    // 指定要擦除的Flash存储体为银行1（Bank 1）
    eraseInitStruct.Banks = FLASH_BANK_1;
    // 将传入的参数Sector赋值给结构体成员，确定要擦除的具体扇区
    eraseInitStruct.Sector = Sector;
    // 表示此次擦除操作只针对1个扇区进行
    eraseInitStruct.NbSectors = 1;
    // 设置擦除操作对应的电压范围为3
    eraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    // 解锁Flash
    status = HAL_FLASH_Unlock();
    if (status!= HAL_OK)
    {
        printf("Flash Unlock Error\r\n");
        return;
    }

    //执行Flash擦除操作
    status = HAL_FLASHEx_Erase(&eraseInitStruct, &pageError);
    if (status!= HAL_OK)
    {
        printf("Flash Erase Error, pageError = %u\r\n",pageError);
        HAL_FLASH_Lock();
        return;
    }

    // 擦除操作完成后，锁定Flash
    HAL_FLASH_Lock();
}

// 用于向Flash中写入应用程序固件数据的函数
// 参数APP_FLAG_ADDRESS是要写入的起始地址，firmware_buffer是指向固件数据缓冲区的指针，firmware_size是固件数据的大小（字节数）
void APP_Flash_Write(uint32_t APP_FLAG_ADDRESS,uint8_t *firmware_buffer,uint32_t firmware_size)
{
    HAL_StatusTypeDef status;

    // 解锁Flash
    status = HAL_FLASH_Unlock();
    if (status!= HAL_OK)
    {
        printf("Flash Unlock Error\r\n");
        return;
    }

    // 通过循环逐个字节地将固件数据写入Flash中
    for (uint32_t i = 0; i < firmware_size; i++)
    {
        // 调用HAL_FLASH_Program函数以字节为单位进行写入操作
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, APP_FLAG_ADDRESS + i, firmware_buffer[i]);
        if (status!= HAL_OK)
        {
            printf("Flash Program Error at address 0x%X\r\n", APP2_FLAG_ADDRESS + i);
            HAL_FLASH_Lock();
            return;
        }
    }

    // 所有字节数据写入完成后，锁定Flash，防止数据异常
    HAL_FLASH_Lock();
}

// 用于一次性更新应用程序固件的函数，整合了擦除、写入以及更新Bootloader标志等操作
void APP_Firmware_Updata(uint32_t APP_FLAG_ADDRESS,uint8_t *firmware_buffer,uint32_t firmware_size)
{
    // 先调用APP_Flash_Erase函数擦除指定的Flash扇区（这里传入扇区编号6）
    APP_Flash_Erase(6);

    // 再调用APP_Flash_Write函数将固件数据写入到指定的APP_FLAG_ADDRESS地址处
    APP_Flash_Write(APP_FLAG_ADDRESS,firmware_buffer,firmware_size);

		// 更新Bootloader标志位为2，表示切换为新的固件
    Update_Bootloader_Flag(2,0x02);
	
		// 下方操作二选一
	
		// 操作一:执行系统复位操作，使系统基于新的固件等配置重新启动
    NVIC_SystemReset();
		// 操作二:执行系统复位操作，使系统基于新的固件等配置重新启动
//		JumpToApplication(APP_FLAG_ADDRESS);
}
