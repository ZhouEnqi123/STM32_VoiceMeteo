/* MyRTC.h - 简单 RTC 封装，用于获取 HH:MM:SS 格式时间字符串 */
#ifndef __MYRTC_H
#define __MYRTC_H

#include <stdint.h>

int MyRTC_Init(void);
/**
 * @brief 获取当前时间字符串（格式 HH:MM:SS）
 * @param buf 输出缓冲区
 * @param len 缓冲区长度（建议 >=9）
 * @return 0 成功，-1 失败（例如 RTC 未就绪）
 */
int MyRTC_GetTimeString(char *buf, uint8_t len);

/**
 * @brief 获取当前日期字符串（格式 YY-MM-DD，例如 26-01-01）
 * @param buf 输出缓冲区
 * @param len 缓冲区长度（建议 >=9）
 * @return 0 成功，-1 失败
 */
int MyRTC_GetDateString(char *buf, uint8_t len);

#endif // __MYRTC_H
