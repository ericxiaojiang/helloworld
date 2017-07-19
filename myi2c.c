#include "myi2c.h"
#include "common.h"
#include "gpio.h"

//I2C引脚初始化
void IIC_Init(void)
{
	   GPIO_QuickInit(HW_GPIOC, 10, kGPIO_Mode_OPP);  //PC10推挽输出 SCL
	   GPIO_QuickInit(HW_GPIOC, 11, kGPIO_Mode_OPP);  //PC11推挽输出 SDA
	
	   /* 控制PTC端口的10和11引脚输出高电平 */
     GPIO_WriteBit(HW_GPIOC, 10, 1);
	   GPIO_WriteBit(HW_GPIOC, 11, 1);
}


//产生IIC起始信号
void IIC_Start(void)
{
		SDA_OUT();     					//SDA线输出
		IIC_SDA_1;	  	        //SDA高  
		IIC_SCL_1;              //SCL高
		DelayUs(4);
		IIC_SDA_0;							//START:when CLK is high,DATA change form high to low 
		DelayUs(4);
		IIC_SCL_0;							//钳住I2C总线，准备发送或接收数据 
}	 


//产生IIC停止信号
void IIC_Stop(void)
{
		SDA_OUT();							//SDA线输出
		IIC_SCL_0;	
		IIC_SDA_0;							//STOP:when CLK is high DATA change form low to high
		DelayUs(4);						
		IIC_SCL_1; 
		IIC_SDA_1;							//发送I2C总线结束信号						   	
}


//返回值为0,接收应答成功
//返回值为1,接收应答失败
bool IIC_WaitAck(void)
{
    uint8_t ack;
    SDA_IN();
    IIC_SCL_0;
    
    DelayUs(1);
    IIC_SCL_1;
    DelayUs(1);
    ack = READ_SDA;   //读取总线上从器件发送的ACK
    IIC_SCL_0;
    SDA_OUT();
    
    return ack;
}

//产生ACK应答
void IIC_Ack(void)
{
		IIC_SCL_0;
		SDA_OUT();
		IIC_SDA_0;
		DelayUs(2);
		IIC_SCL_1;
		DelayUs(2);
		IIC_SCL_0;
}

//不产生ACK应答		    
void IIC_NAck(void)																																							
{
		IIC_SCL_0;
		SDA_OUT();
		IIC_SDA_1;
		DelayUs(2);
		IIC_SCL_1;
		DelayUs(2);
		IIC_SCL_0;
}			


//I2C发送一个字节
void I2C_SendByte(uint8_t data)
{
    uint8_t i;
    SDA_OUT(); 	 
    i = 8;
    while(i--)
    {
        if(data & 0x80) IIC_SDA_1;
        else IIC_SDA_0;
        data <<= 1;
        DelayUs(1);
        IIC_SCL_1;
        DelayUs(1);
        IIC_SCL_0;
    }

}


//I2C读一个字节
uint8_t I2C_ReadByte(void)
{
    uint8_t i,byte;
    
	 // i = 8;
    byte = 0;

    SDA_IN();
	
//    while(i--)
//    {
//        IIC_SCL_0;
//        DelayUs(1);
//        IIC_SCL_1;
//        DelayUs(1);
//        byte = (byte<<1)|(READ_SDA & 1);
//    }

	  for(i=0;i < 8;i++)
	  {
				IIC_SCL_0;
        DelayUs(1);
        IIC_SCL_1;
			  byte<<=1;
			  if(READ_SDA)
				{
						byte++;
				}
				DelayUs(1);
		}
    IIC_SCL_0;
    SDA_OUT();
    return byte;
}

/*
 * 写多字节函数
 * 返回值 0 success
 * 返回值 1 failure
*/
int I2C_MutipleWrite(uint32_t instance ,uint8_t chipAddr, uint32_t addr, uint32_t addrLen, uint8_t *buf, uint32_t len)
{
    uint8_t *p;
    uint8_t err;
    
    p = (uint8_t*)&addr;
    err = 0;
    chipAddr <<= 1;
    
    IIC_Start();
    I2C_SendByte(chipAddr);
    err += IIC_WaitAck();

    while(addrLen--)
    {
        I2C_SendByte(*p++);
        err += IIC_WaitAck();   //等待到的所有ACK应答得累加,若所有应答为0,success
    }
    while(len--)
    {
        I2C_SendByte(*buf++);
        err += IIC_WaitAck();   //等待到的所有ACK应答得累加,若所有应答为0,success
    }
    IIC_Stop();
    return err;
}

//写单个寄存器数据
int I2C_WriteSingleRegister(uint32_t instance, uint8_t chipAddr, uint8_t addr, uint8_t data)
{
    return I2C_MutipleWrite(instance, chipAddr, addr, 1, &data, 1);
}


/*
 * 读取多字节函数
 * 返回值 0 success
 * 返回值 1 failure
*/
int I2C_MutipleRead(uint32_t instance ,uint8_t chipAddr, uint32_t addr, uint32_t addrLen, uint8_t *buf, uint32_t len)
{
    uint8_t *p;
    uint8_t err;
    
    p = (uint8_t*)&addr;
    err = 0;
    chipAddr <<= 1;             //高7位为器件地址
    
    IIC_Start();
    I2C_SendByte(chipAddr);     //发送写命令
    err += IIC_WaitAck();
    
    while(addrLen--)
    {
        I2C_SendByte(*p++);
        err += IIC_WaitAck();
    }
    
    IIC_Start();
    I2C_SendByte(chipAddr+1);    //发送读命令
    err += IIC_WaitAck();
    
    while(len--)
    {
        *buf++ = I2C_ReadByte();
        if(len)
        {
            IIC_Ack();
        }
    }
    
    IIC_NAck();
    IIC_Stop();
    
    return err;
}


//读取单个寄存器的数据
int I2C_ReadSingleRegister(uint32_t instance, uint8_t chipAddr, uint8_t addr, uint8_t *data)
{
    return I2C_MutipleRead(instance, chipAddr, addr, 1, data, 1);
}


/*
 * check slaver
 * 返回值 0 success
 * 返回值 1 failure
*/
int I2C_Probe(uint32_t instance, uint8_t chipAddr)
{
    uint8_t err;
    
    err = 0;
    chipAddr <<= 1;
    
    IIC_Start();
    I2C_SendByte(chipAddr);
    err = IIC_WaitAck();
    IIC_Stop();
    return err;
}


//Slaver IC初始化函数
void AT24CXX_Init(void)
{
	IIC_Init();
}

/*
 * 写EEPROM函数
 * 返回值 0 success
 * 返回值 1 failure
*/
int AT24CX_write_page(uint32_t addr, uint8_t *buf, uint32_t len)
{
    int ret;
    uint8_t chip_addr;
    
    chip_addr = (addr/256) + 0x50;
    ret = I2C_MutipleWrite(0, chip_addr, addr%256, 2, buf, len);  //24C08(寄存器占两个字节)
    while(I2C_Probe(0,0x50));
    return ret;
}

/*
 * 读EEPROM函数
 * 返回值 0 success
 * 返回值 1 failure
*/
int AT24CX_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t chip_addr;
    
    chip_addr = (addr/256) + 0x50;
    return I2C_MutipleRead(0, chip_addr, addr%256, 2, buf, len);    //24C08(寄存器占两个字节)
}










