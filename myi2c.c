#include "myi2c.h"
#include "common.h"
#include "gpio.h"

//I2C���ų�ʼ��
void IIC_Init(void)
{
	   GPIO_QuickInit(HW_GPIOC, 10, kGPIO_Mode_OPP);  //PC10������� SCL
	   GPIO_QuickInit(HW_GPIOC, 11, kGPIO_Mode_OPP);  //PC11������� SDA
	
	   /* ����PTC�˿ڵ�10��11��������ߵ�ƽ */
     GPIO_WriteBit(HW_GPIOC, 10, 1);
	   GPIO_WriteBit(HW_GPIOC, 11, 1);
}


//����IIC��ʼ�ź�
void IIC_Start(void)
{
		SDA_OUT();     					//SDA�����
		IIC_SDA_1;	  	        //SDA��  
		IIC_SCL_1;              //SCL��
		DelayUs(4);
		IIC_SDA_0;							//START:when CLK is high,DATA change form high to low 
		DelayUs(4);
		IIC_SCL_0;							//ǯסI2C���ߣ�׼�����ͻ�������� 
}	 


//����IICֹͣ�ź�
void IIC_Stop(void)
{
		SDA_OUT();							//SDA�����
		IIC_SCL_0;	
		IIC_SDA_0;							//STOP:when CLK is high DATA change form low to high
		DelayUs(4);						
		IIC_SCL_1; 
		IIC_SDA_1;							//����I2C���߽����ź�						   	
}


//����ֵΪ0,����Ӧ��ɹ�
//����ֵΪ1,����Ӧ��ʧ��
bool IIC_WaitAck(void)
{
    uint8_t ack;
    SDA_IN();
    IIC_SCL_0;
    
    DelayUs(1);
    IIC_SCL_1;
    DelayUs(1);
    ack = READ_SDA;   //��ȡ�����ϴ��������͵�ACK
    IIC_SCL_0;
    SDA_OUT();
    
    return ack;
}

//����ACKӦ��
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

//������ACKӦ��		    
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


//I2C����һ���ֽ�
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


//I2C��һ���ֽ�
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
 * д���ֽں���
 * ����ֵ 0 success
 * ����ֵ 1 failure
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
        err += IIC_WaitAck();   //�ȴ���������ACKӦ����ۼ�,������Ӧ��Ϊ0,success
    }
    while(len--)
    {
        I2C_SendByte(*buf++);
        err += IIC_WaitAck();   //�ȴ���������ACKӦ����ۼ�,������Ӧ��Ϊ0,success
    }
    IIC_Stop();
    return err;
}

//д�����Ĵ�������
int I2C_WriteSingleRegister(uint32_t instance, uint8_t chipAddr, uint8_t addr, uint8_t data)
{
    return I2C_MutipleWrite(instance, chipAddr, addr, 1, &data, 1);
}


/*
 * ��ȡ���ֽں���
 * ����ֵ 0 success
 * ����ֵ 1 failure
*/
int I2C_MutipleRead(uint32_t instance ,uint8_t chipAddr, uint32_t addr, uint32_t addrLen, uint8_t *buf, uint32_t len)
{
    uint8_t *p;
    uint8_t err;
    
    p = (uint8_t*)&addr;
    err = 0;
    chipAddr <<= 1;             //��7λΪ������ַ
    
    IIC_Start();
    I2C_SendByte(chipAddr);     //����д����
    err += IIC_WaitAck();
    
    while(addrLen--)
    {
        I2C_SendByte(*p++);
        err += IIC_WaitAck();
    }
    
    IIC_Start();
    I2C_SendByte(chipAddr+1);    //���Ͷ�����
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


//��ȡ�����Ĵ���������
int I2C_ReadSingleRegister(uint32_t instance, uint8_t chipAddr, uint8_t addr, uint8_t *data)
{
    return I2C_MutipleRead(instance, chipAddr, addr, 1, data, 1);
}


/*
 * check slaver
 * ����ֵ 0 success
 * ����ֵ 1 failure
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


//Slaver IC��ʼ������
void AT24CXX_Init(void)
{
	IIC_Init();
}

/*
 * дEEPROM����
 * ����ֵ 0 success
 * ����ֵ 1 failure
*/
int AT24CX_write_page(uint32_t addr, uint8_t *buf, uint32_t len)
{
    int ret;
    uint8_t chip_addr;
    
    chip_addr = (addr/256) + 0x50;
    ret = I2C_MutipleWrite(0, chip_addr, addr%256, 2, buf, len);  //24C08(�Ĵ���ռ�����ֽ�)
    while(I2C_Probe(0,0x50));
    return ret;
}

/*
 * ��EEPROM����
 * ����ֵ 0 success
 * ����ֵ 1 failure
*/
int AT24CX_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t chip_addr;
    
    chip_addr = (addr/256) + 0x50;
    return I2C_MutipleRead(0, chip_addr, addr%256, 2, buf, len);    //24C08(�Ĵ���ռ�����ֽ�)
}










