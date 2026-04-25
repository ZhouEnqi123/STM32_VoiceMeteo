/* Clock.h - 简易闹钟驱动接口
 * 按键映射（外部中断下降沿触发）：
 *  - PA0 = 按键1（开/关闹钟；设置时切换字段/保存；响铃时停止响铃）
 *  - PB1 = 按键2（进入设置/设置中为当前字段+1；响铃时停止响铃）
 * 蜂鸣器：PB8，高电平有效。
 */
#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>

void Clock_Init(void);

/* 周期性调用（主循环或 1Hz 定时器）用于响铃节奏与触发检测 */
void Clock_Periodic(void);

/* 在 EXTI 回调中调用（下降沿） */
void Clock_PA0_Pressed(void);
void Clock_PB1_Pressed(void);

/* 直接控制/查询 */
void Clock_SetAlarm(uint8_t hour, uint8_t minute);
void Clock_GetAlarm(uint8_t *enabled, uint8_t *hour, uint8_t *minute);
void Clock_ToggleEnable(void);
void Clock_StopRinging(void);

/* 设置态查询，用于 UI 冻结与闪烁 */
uint8_t Clock_IsSetting(void);
void Clock_GetTemp(uint8_t *hour, uint8_t *minute);
uint8_t Clock_GetSettingField(void); /* 0=hour,1=minute */
uint8_t Clock_GetFlashState(void); /* 0/1 闪烁状态 */

#endif // __CLOCK_H__
