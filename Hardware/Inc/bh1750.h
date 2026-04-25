/* ============================== BH1750 驱动 =============================== */
/*                             注释风格：居中对齐                             */
#ifndef __BH1750_H__
#define __BH1750_H__

#include <stdint.h>

/*  BH1750 常用 API:
 *  - BH1750_Init()     : 初始化设备并进入连续高分辨率测量模式
 *  - BH1750_ReadLux()  : 读取并返回当前光照强度（单位：lx）
 *  - BH1750_IsReady()  : 检查设备在 I2C 总线上是否存在
 */

void BH1750_Init(void);
int  BH1750_ReadLux(float *lux);
int  BH1750_IsReady(void);

#endif /* __BH1750_H__ */
