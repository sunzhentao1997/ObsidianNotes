# 一、串口数据收发篇
## ①串口双缓存接收(空闲中断)

STMF103空闲中断双缓存接收，由于HAL库中未对空闲回调函数进行调用，使用时需在串口中断处理中进行调用并判断串口空闲标志位是否置位。
```c
uint8_t RecvBUff1[128] = {0};
uint8_t RecvBuff2[128] = {0};
uint8_t *active_buff = RecvBuff1;
uint8_t *passive_buff = RecvBuff2;

void HAL_UART_IdleCallback(UART_HandleTypeDef *huart)
{
    uint8_t cmd_id = 0;
    
    if((huart->Instance == USART3) && 
       (RESET != AL_UART_GET_FLAG(&huart3,UART_FLAG_IDLE)))
    {
        uint8_t *temp_buff = passive_buff；
        passive_buff = active_buff；
        active_buff = temp_buff；
            
        /*串口数据处理*/
            
       HAL_UART_Receive_DMA(&huart3，active_buff，128)；
    }
}

int main(void)
{
	HAL_UART_Receive_DMA(&huart3，active_buff，128)；    /*初始化开启串口DMA输出*/
    
    While(1)
    {
        
    }
}
```

## ②串口不定长度接收(中断+DMA)

使用“HAL_UARTEx_RxEventCallback”回调函数可以处理非固定长度的串口数据，但接收数据的数组需要足够大或者关闭过半中断，否则当一次接受的数据长度超过数组的一半时会再次触发中断；接收的指令格式建议: 帧头 + 数据长度 + 数据指令 + 帧尾；
```c
uint8_t RecvBUff[128] = {0};

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART3)
    {
        HAL_UART_Transmit_DMA(&huart3，RecvBuff，Size)；
            
        /*
         *			处理接收数据
         */
        
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3，RecvBUff，sizeof(RecvBuff))；
        __HAL_DMA_DISABLE_IT(&hdma_usart3_rx,DMA_IT_HT)；
    }
}

int main(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3，RecvBUff，sizeof(RecvBuff))；
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx,DMA_IT_HT)；				//关闭过半中断
        
    while(1)
    {
        
    }
}
```

## ③ 串口接收数据处理

配合②进行数据处理

```c
// 指令的最小长度  
#define COMMAND_MIN_LENGTH 4  
  
// 循环缓冲区大小  
#define BUFFER_SIZE 128  
// 循环缓冲区  
uint8_t buffer[BUFFER_SIZE];  
// 循环缓冲区读索引  
uint8_t readIndex = 0;  
// 循环缓冲区写索引  
uint8_t writeIndex = 0;  
  
/**  
* @brief 增加读索引  
* @param length 要增加的长度  
*/  
void Command_AddReadIndex(uint8_t length) {  
	readIndex += length;  
	readIndex %= BUFFER_SIZE;  
}  
  
/**  
* @brief 读取第i位数据 超过缓存区长度自动循环  
* @param i 要读取的数据索引  
*/  
  
uint8_t Command_Read(uint8_t i) {  
	uint8_t index = i % BUFFER_SIZE;  
	return buffer[index];  
}  
  
/**  
* @brief 计算未处理的数据长度  
* @return 未处理的数据长度  
* @retval 0 缓冲区为空  
* @retval 1~BUFFER_SIZE-1 未处理的数据长度  
* @retval BUFFER_SIZE 缓冲区已满  
*/  
//uint8_t Command_GetLength() {  
// // 读索引等于写索引时，缓冲区为空  
// if (readIndex == writeIndex) {  
// return 0;  
// }  
// // 如果缓冲区已满,返回BUFFER_SIZE  
// if (writeIndex + 1 == readIndex || (writeIndex == BUFFER_SIZE - 1 && readIndex == 0)) {  
// return BUFFER_SIZE;  
// }  
// // 如果缓冲区未满,返回未处理的数据长度  
// if (readIndex <= writeIndex) {  
// return writeIndex - readIndex;  
// } else {  
// return BUFFER_SIZE - readIndex + writeIndex;  
// }  
//}  
  
uint8_t Command_GetLength() {  
	return (writeIndex + BUFFER_SIZE - readIndex) % BUFFER_SIZE;  
}  
  
  
/**  
* @brief 计算缓冲区剩余空间  
* @return 剩余空间  
* @retval 0 缓冲区已满  
* @retval 1~BUFFER_SIZE-1 剩余空间  
* @retval BUFFER_SIZE 缓冲区为空  
*/  
uint8_t Command_GetRemain() {  
	return BUFFER_SIZE - Command_GetLength();  
}  
  
/**  
* @brief 向缓冲区写入数据  
* @param data 要写入的数据指针  
* @param length 要写入的数据长度  
* @return 写入的数据长度  
*/  
uint8_t Command_Write(uint8_t *data, uint8_t length) {  
	// 如果缓冲区不足 则不写入数据 返回0  
	if (Command_GetRemain() < length) {  
		return 0;  
	}  
	// 使用memcpy函数将数据写入缓冲区  
	if (writeIndex + length < BUFFER_SIZE) {  
		memcpy(buffer + writeIndex, data, length);  
		writeIndex += length;  
	} else {  
		uint8_t firstLength = BUFFER_SIZE - writeIndex;  
		memcpy(buffer + writeIndex, data, firstLength);  
		memcpy(buffer, data + firstLength, length - firstLength);  
		writeIndex = length - firstLength;  
	}  
	return length;  
}  
  
/**  
* @brief 尝试获取一条指令  
* @param command 指令存放指针  
* @return 获取的指令长度  
* @retval 0 没有获取到指令  
*/  
uint8_t Command_GetCommand(uint8_t *command) 
{  
	// 寻找完整指令  
	while (1) {  
	// 如果缓冲区长度小于COMMAND_MIN_LENGTH 则不可能有完整的指令  
		if (Command_GetLength() < COMMAND_MIN_LENGTH) {  
			return 0;  
		}  
		// 如果不是包头 则跳过 重新开始寻找  
		if (Command_Read(readIndex) != 0xAA) {  
			Command_AddReadIndex(1);  
			continue;  
		}  
		// 如果缓冲区长度小于指令长度 则不可能有完整的指令  
		uint8_t length = Command_Read(readIndex + 1);  
		if (Command_GetLength() < length) {  
			return 0;  
		}  
		// 如果校验和不正确 则跳过 重新开始寻找  
		uint8_t sum = 0;  
		for (uint8_t i = 0; i < length - 1; i++) {  
			sum += Command_Read(readIndex + i);  
		}  
		if (sum != Command_Read(readIndex + length - 1)) {  
			Command_AddReadIndex(1);  
			continue;  
		}  
		// 如果找到完整指令 则将指令写入command 返回指令长度  
		for (uint8_t i = 0; i < length; i++) {  
			command[i] = Command_Read(readIndex + i);  
		}  
		Command_AddReadIndex(length);  
		return length;  
	}  
}
```

# 二、ADC采样篇

## ①  ADC多通道常规采样 + DMA

### 1、ADC配置
![](pic/ADC规则组配置.png)
# 二、芯片加密篇
	
## ① 获取单片机UID

```c
uint32_t UID[3] = {0};

void GetSTM32F103_UID(void)
{
	UID[0] = HAL_GetUIDw0();
    UID[1] = HAL_GetUIDw1();
    UID[2] = HAL_GetUIDw2();
}
```

## ② MD5算法

### MD5.h
```c
#ifndef MD5_H
#define MD5_H
 
typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];   
}MD5_CTX;
 
                         
#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))
#define FF(a,b,c,d,x,s,ac) \
          { \
          a += F(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define GG(a,b,c,d,x,s,ac) \
          { \
          a += G(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define HH(a,b,c,d,x,s,ac) \
          { \
          a += H(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define II(a,b,c,d,x,s,ac) \
          { \
          a += I(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }                                            
void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen);
void MD5Final(MD5_CTX *context,unsigned char digest[16]);
void MD5Transform(unsigned int state[4],unsigned char block[64]);
void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len);
void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len);
 
#endif
```
### MD5.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "md5.h"

unsigned char PADDING[]={0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                         
void MD5Init(MD5_CTX *context)
{
     context->count[0] = 0;
     context->count[1] = 0;
     context->state[0] = 0x67452301;
     context->state[1] = 0xEFCDAB89;
     context->state[2] = 0x98BADCFE;
     context->state[3] = 0x10325476;
}
void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen)
{
    unsigned int i = 0,index = 0,partlen = 0;
    index = (context->count[0] >> 3) & 0x3F;
    partlen = 64 - index;
    context->count[0] += inputlen << 3;
    if(context->count[0] < (inputlen << 3))
       context->count[1]++;
    context->count[1] += inputlen >> 29;
    
    if(inputlen >= partlen)
    {
       memcpy(&context->buffer[index],input,partlen);
       MD5Transform(context->state,context->buffer);
       for(i = partlen;i+64 <= inputlen;i+=64)
           MD5Transform(context->state,&input[i]);
       index = 0;        
    }  
    else
    {
        i = 0;
    }
    memcpy(&context->buffer[index],&input[i],inputlen-i);
}
void MD5Final(MD5_CTX *context,unsigned char digest[16])
{
    unsigned int index = 0,padlen = 0;
    unsigned char bits[8];
    index = (context->count[0] >> 3) & 0x3F;
    padlen = (index < 56)?(56-index):(120-index);
    MD5Encode(bits,context->count,8);
    MD5Update(context,PADDING,padlen);
    MD5Update(context,bits,8);
    MD5Encode(digest,context->state,16);
}
void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len)
{
    unsigned int i = 0,j = 0;
    while(j < len)
    {
        output[j] = input[i] & 0xFF;  
        output[j+1] = (input[i] >> 8) & 0xFF;
        output[j+2] = (input[i] >> 16) & 0xFF;
        output[j+3] = (input[i] >> 24) & 0xFF;
        i++;
        j+=4;
    }
}
void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len)
{
     unsigned int i = 0,j = 0;
     while(j < len)
     {
        output[i] = (input[j]) |
                    (input[j+1] << 8) |
                    (input[j+2] << 16) |
                    (input[j+3] << 24);
        i++;
        j+=4; 
     }
}
void MD5Transform(unsigned int state[4],unsigned char block[64])
{
    unsigned int a = state[0];
    unsigned int b = state[1];
    unsigned int c = state[2];
    unsigned int d = state[3];
    unsigned int x[64];
    MD5Decode(x,block,64);
    FF(a, b, c, d, x[ 0], 7, 0xd76aa478); /* 1 */
    FF(d, a, b, c, x[ 1], 12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[ 2], 17, 0x242070db); /* 3 */
    FF(b, c, d, a, x[ 3], 22, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[ 4], 7, 0xf57c0faf); /* 5 */
    FF(d, a, b, c, x[ 5], 12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[ 6], 17, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[ 7], 22, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[ 8], 7, 0x698098d8); /* 9 */
    FF(d, a, b, c, x[ 9], 12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], 17, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], 22, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], 7, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], 12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], 17, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], 22, 0x49b40821); /* 16 */
 
    /* Round 2 */
    GG(a, b, c, d, x[ 1], 5, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[ 6], 9, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], 14, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[ 5], 5, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], 9,  0x2441453); /* 22 */
    GG(c, d, a, b, x[15], 14, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[ 9], 5, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], 9, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[ 3], 14, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[ 8], 20, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], 5, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[ 2], 9, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[ 7], 14, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[ 5], 4, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[ 8], 11, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], 16, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], 23, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[ 1], 4, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], 23, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], 4, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[ 0], 11, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[ 3], 16, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[ 6], 23,  0x4881d05); /* 44 */
    HH(a, b, c, d, x[ 9], 4, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], 11, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[ 2], 23, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[ 0], 6, 0xf4292244); /* 49 */
    II(d, a, b, c, x[ 7], 10, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], 15, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[ 5], 21, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], 6, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[ 3], 10, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], 15, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[ 1], 21, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[ 8], 6, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[ 6], 15, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], 21, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[ 4], 6, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], 10, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[ 9], 21, 0xeb86d391); /* 64 */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void main( void ) 
{ 
    int read_len;
    int i ;
    char temp[16]={0};
    unsigned int uid_data[3] = {0x066AFF30, 0x35505030, 0x43243328};
    unsigned char decrypt[16]={0};  
    unsigned char decrypt32[64]={0};    
    unsigned char decrypt32_temp[64]={0}; 
    
    MD5_CTX md5c; 

    for(i=0;i<3;i++)
    {
        sprintf(temp,"%08X",uid_data[i]);
        //itoa(uid_data[i],temp,16);
        strcat((char *)decrypt32,temp);
        printf("decrypt32 = %s\n",decrypt32);
    }

    MD5Init(&md5c); //初始化
    read_len = strlen((const char*)decrypt32);
    MD5Update(&md5c,(unsigned char *)decrypt32,read_len);  
  
    MD5Final(&md5c,decrypt); 
    strcpy((char *)decrypt32_temp,"");

    for(i=0;i<16;i++)
    {
        sprintf(temp,"%02x",decrypt[i]);
        strcat((char *)decrypt32_temp,temp);
    }

    while(1)
    {
        printf("md5:%s\n",decrypt32_temp);
        printf("read_len = %d\n",read_len);
        sleep(5);
    }

    return;
}
```

[^1]: 
