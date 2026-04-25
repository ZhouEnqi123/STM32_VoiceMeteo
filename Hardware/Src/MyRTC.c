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

int MyRTC_GetDateString(char *buf, uint8_t len)
{
  if (buf == NULL || len < 9) return -1;

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  /* 读取时间以满足 HAL 要求，随后读取日期 */
  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    return -1;
  if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    return -1;

  /* 格式 YY-MM-DD，例如 26-01-01（hrtc Date.Year 为 0..99） */
  snprintf(buf, len, "%02u-%02u-%02u", (unsigned)sDate.Year, (unsigned)sDate.Month, (unsigned)sDate.Date);
  return 0;
}
