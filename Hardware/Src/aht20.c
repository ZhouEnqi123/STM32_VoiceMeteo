/**
 * @file aht20.c
 * @brief 波特律动AHT20(DHT20)驱动
 * @anchor 波特律动(keysking 博哥在学习)
 * @version 1.0
 * @date 2023-08-21
 * @license MIT License
 *
 * @attention
 * 本驱动库基于波特律动·keysking的STM32教程学习套件进行开发
 * 在其他硬件平台上使用可能需要进行简单地移植: 修改Send与Reveive函数
 *
 * @note
 * 使用流程:
 * 1. STM32初始化IIC完成后调用AHT20_Init()初始化AHT20.
 * 2. 调用AHT20_Measure()可以进行一次测量
 * 3. 调用AHT20_Temperature()与AHT20_Humidity()分别可以获取上次测量时的温度与湿度数据
 */
#include "aht20.h"
#include "i2c.h"

/* ============================ AHT20 I2C 地址 =============================== */
/* 7-bit 地址: 0x38  -> HAL 传输函数使用左移后地址 (0x38 << 1) = 0x70 */
#define AHT20_ADDRESS 0x70

// 温湿度数据暂存变量
float Temperature;
float Humidity;

// ========================== 底层通信函数 ==========================

/**
 * @brief 向AHT20发送数据的函数
 * @param data 发送的数据
 * @param len 发送的数据长度
 * @return void
 * @note 此函数是移植本驱动时的重要函数 将本驱动库移植到其他平台时应根据实际情况修改此函数
 */
void AHT20_Send(uint8_t *data, uint8_t len) {
  HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDRESS, data, len, HAL_MAX_DELAY);
}

/**
 * @brief 从AHT20接收数据的函数
 * @param data 接收数据的缓冲区
 * @param len 接收数据的长度
 * @return void
 * @note 此函数是移植本驱动时的重要函数 将本驱动库移植到其他平台时应根据实际情况修改此函数
 */
void AHT20_Receive(uint8_t *data, uint8_t len) {
  HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, data, len, HAL_MAX_DELAY);
}

/**
 * @brief 读取原始数据（至少 7 字节）
 * @param buf 缓冲区
 * @param len 要读取的字节数（建议 7）
 * @return 0 成功, -1 失败
 */
int AHT20_ReadRaw(uint8_t *buf, uint8_t len) {
  if (buf == NULL || len < 1) return -1;
  if (HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDRESS, buf, len, HAL_MAX_DELAY) != HAL_OK) return -1;
  return 0;
}

/**
 * @brief 检查 AHT20 设备是否在 I2C 总线上可用（低级检测）
 * @return 0: 可用, -1: 不可用
 */
int AHT20_IsReady(void)
{
  if (HAL_I2C_IsDeviceReady(&hi2c1, AHT20_ADDRESS, 3, 50) == HAL_OK)
    return 0;
  return -1;
}

/**
 * @brief 对 AHT20 发送软复位命令
 * @return 0: 成功, -1: 失败
 */
/* Soft reset removed per user request (AHT20 OFF handling in main.c) */

/**
 * @brief AHT20初始化函数
 */
void AHT20_Init() {
  /* -------------------- 上电延时并检查设备 -------------------- */
  HAL_Delay(40); // 上电后至少等待 40ms

  if (AHT20_IsReady() != 0) {
    /* 设备不可用，返回由调用者决定后续处理 */
    return;
  }

  /* 读取状态字，检查 CAL 位（Bit3） */
  uint8_t status = 0;
  if (AHT20_ReadRaw(&status, 1) != 0) {
    return; // 读取失败则直接返回
  }

  /* 若未校准则发送初始化命令 (0xBE, 0x08, 0x00) 并短延时 */
  if ((status & 0x08) == 0x00) {
    uint8_t sendBuffer[3] = {0xBE, 0x08, 0x00};
    AHT20_Send(sendBuffer, 3);
    HAL_Delay(10);
  }
}

/**
 * @brief AHT20测量函数
 * @note 测量完成后可以通过AHT20_Temperature()与AHT20_Humidity()获取温度与湿度数据
 */
void AHT20_Measure() {
  uint8_t sendBuffer[3] = {0xAC, 0x33, 0x00};
  uint8_t readBuffer[7] = {0};

  // 触发测量
  AHT20_Send(sendBuffer, 3);
  // 等待转换完成，官方建议 >=80ms
  HAL_Delay(80);

  /* 读取 7 字节（含状态字），若读取失败则直接返回 */
  if (AHT20_ReadRaw(readBuffer, 7) != 0) {
    return; // 读取失败
  }

  /* 状态字 Bit7 为忙标志，若为0表示准备好 */
  if ((readBuffer[0] & 0x80) == 0x00) {
    /* 按说明书组合 20 位原始数据 */
    uint32_t S_rh = ((uint32_t)readBuffer[1] << 12) | ((uint32_t)readBuffer[2] << 4) | ((uint32_t)readBuffer[3] >> 4);
    uint32_t S_t = (((uint32_t)readBuffer[3] & 0x0F) << 16) | ((uint32_t)readBuffer[4] << 8) | (uint32_t)readBuffer[5];

    /* 计算并保存温湿度 */
    Humidity = (float)S_rh * 100.0f / (float)(1 << 20);
    Temperature = (float)S_t * 200.0f / (float)(1 << 20) - 50.0f;
  }
}

/**
 * @brief 获取上次测量时的温度数据
 */
float AHT20_Temperature() {
  return Temperature;
}

/**
 * @brief 获取上次测量时的湿度数据
 */
float AHT20_Humidity() {
  return Humidity;
}
