/**
 * @file    bh1750.c
 * @brief   BH1750 light sensor driver (I2C)
 *
 * 说明：驱动使用 I2C 通信，默认 ADDR 接地（7-bit = 0x23，HAL 地址 = 0x46）。
 * 本驱动实现：初始化、就绪检查、读取 2 字节原始数据并转换成 lux。
 */

#include "bh1750.h"
#include "i2c.h"
#include <stdint.h>

/* BH1750 I2C 地址（HAL 使用 8-bit 地址 = 7-bit << 1） */
#define BH1750_ADDRESS 0x46 /* 0x23 << 1, ADDR = GND */

/* BH1750 命令 */
#define BH1750_POWER_ON      0x01
#define BH1750_CONT_H_RES    0x10 /* Continuously H-Resolution Mode */

/* 初始化：上电后发送 POWER ON 并进入连续高分辨率测量模式 */
void BH1750_Init(void)
{
  uint8_t cmd;

  /* 上电后确保设备可用 */
  cmd = BH1750_POWER_ON;
  HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDRESS, &cmd, 1, HAL_MAX_DELAY);

  /* 进入连续高分辨率测量模式 */
  cmd = BH1750_CONT_H_RES;
  HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDRESS, &cmd, 1, HAL_MAX_DELAY);
  /* 等待首次转换完成（高分辨率模式约 120ms）以避免首次读到 0 */
  HAL_Delay(180);
}

/* 检查设备是否在总线上可用 */
int BH1750_IsReady(void)
{
  if (HAL_I2C_IsDeviceReady(&hi2c1, BH1750_ADDRESS, 3, 50) == HAL_OK)
    return 0;
  return -1;
}

/* 读取光照强度（lx），返回 0 成功，-1 失败 */
int BH1750_ReadLux(float *lux)
{
  if (lux == NULL) return -1;
  uint8_t buf[2] = {0};
  /* 连续测量模式下读取两字节原始数据
   * 若上电或刚进入测量模式可能返回 0，尝试重试若干次
   */
  const int max_retries = 3;
  for (int i = 0; i < max_retries; ++i) {
    if (HAL_I2C_Master_Receive(&hi2c1, BH1750_ADDRESS, buf, 2, HAL_MAX_DELAY) == HAL_OK) {
      uint16_t raw = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
      if (raw != 0) {
        *lux = (float)raw / 1.2f; /* convert per datasheet */
        return 0;
      }
    }
    /* 等待测量完成时间后重试 */
    HAL_Delay(120);
  }
  /* 都重试失败，返回错误 */
  return -1;
}
