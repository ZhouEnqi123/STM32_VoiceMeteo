/* MyRTC.c - 简单 RTC 封装，用于从 HAL RTC 获取时间字符串 */
#include "MyRTC.h"
#include "rtc.h" // 提供 extern RTC_HandleTypeDef hrtc
#include "stm32f1xx_hal.h"
#include <stdio.h>

int MyRTC_Init(void)
{
  /* 目前不需要额外初始化，MX_RTC_Init 已在 main 中调用 */
  return 0;
}

int MyRTC_GetTimeString(char *buf, uint8_t len)
{
  if (buf == NULL || len < 9) return -1;

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    return -1;
  /* 读取日期以清除寄存器锁（HAL 要求：读取时间后应读取日期） */
  (void)HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  /* 格式化为 HH:MM:SS */
  int hh = sTime.Hours;
  int mm = sTime.Minutes;
  int ss = sTime.Seconds;
  snprintf(buf, len, "%02d:%02d:%02d", hh, mm, ss);
  return 0;
}
