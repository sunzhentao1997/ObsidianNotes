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
uint8_t Command_GetCommand(uint8_t *command) {  
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