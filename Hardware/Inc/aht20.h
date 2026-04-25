#ifndef __AHT20_H__
#define __AHT20_H__

#/**************************** AHT20 driver ******************************/
#/*                      Comment style: centered                         */
#include <stdint.h>
#include <stddef.h>

/*  AHT20 常用 API:
 *  - AHT20_Init()        : 初始化（含校准检查）
 *  - AHT20_Measure()     : 触发测量并读取计算温湿度
 *  - AHT20_ReadRaw(...)  : 读取原始字节（建议 7 字节）
 *  - AHT20_IsReady()     : check device presence on I2C bus
 */

void AHT20_Init();
void AHT20_Measure();
float AHT20_Temperature();
float AHT20_Humidity();
int AHT20_ReadRaw(uint8_t *buf, uint8_t len);
int AHT20_IsReady(void);

#endif
