/* Clock.c - 简易闹钟驱动实现
 * 说明：
 * - 备份寄存器使用 RTC_BKP_DR2 保存设置（bit0=enabled; bits8-15 hour; bits16-23 minute）
 * - 外部按键通过 PA0/PB1 的 EXTI 下降沿调用 Clock_PA0_Pressed/Clock_PB1_Pressed
 * - 主循环或 1Hz 定时器需要定期调用 Clock_Periodic
 */

#include "Clock.h"
#include "stm32f1xx_hal.h"
#include "rtc.h" /* extern RTC_HandleTypeDef hrtc */

/* 备份寄存器布局 */
static uint8_t alarm_enabled = 0;
static uint8_t alarm_hour = 0;
static uint8_t alarm_min = 0;

/* 设置态临时值与状态 */
static uint8_t setting_mode = 0; /* 0=normal,1=setting */
static uint8_t setting_field = 0; /* 0=hour,1=min */
static uint8_t temp_h = 0, temp_m = 0;
static uint8_t flash_state = 1;

/* 响铃状态与节奏 */
static uint8_t firing = 0;
static uint32_t last_buzz = 0;
static const uint32_t buzz_period = 500; /* ms */

/* 一次性短鸣（按键反馈），到期由 Clock_Periodic 停止 */
static uint32_t oneshot_until = 0;

/* 辅助：触发一次性短鸣（设置 PB8 并设置到期时间） */
static void oneshot_beep(uint32_t ms)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
  oneshot_until = HAL_GetTick() + ms;
}
#define DEBOUNCE_MS 50
typedef enum {BTN_IDLE=0, BTN_DEBOUNCE_DOWN, BTN_PRESSED, BTN_DEBOUNCE_UP} btn_state_t;
static btn_state_t pa0_state = BTN_IDLE;
static btn_state_t pb1_state = BTN_IDLE;
static uint32_t pa0_ts = 0;
static uint32_t pb1_ts = 0;
/* 每键独立冷却，避免按键互相影响（单位 ms） */
static const uint32_t BUTTON_COOLDOWN_MS = 200;
static uint32_t pa0_last_action_ts = 0;
static uint32_t pb1_last_action_ts = 0;

/* 按键防抖：旧的时间戳变量已由状态机替代 */

static void load_bkp(void)
{
  uint32_t v = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
  alarm_enabled = (v & 0x1) ? 1 : 0;
  alarm_hour = (v >> 8) & 0xFF;
  alarm_min = (v >> 16) & 0xFF;
}

static void save_bkp(void)
{
  uint32_t v = 0;
  v |= (alarm_enabled ? 1u : 0u);
  v |= ((uint32_t)alarm_hour & 0xFFu) << 8;
  v |= ((uint32_t)alarm_min & 0xFFu) << 16;
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, v);
}

void Clock_Init(void)
{
  load_bkp();
  firing = 0;
  last_buzz = HAL_GetTick();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); /* 蜂鸣器静音 */
}

void Clock_Periodic(void)
{
  uint32_t now = HAL_GetTick();

  /* 响铃节奏（翻转 PB8）: 保持原有节奏行为，按键短鸣不应改变此逻辑 */
  if (firing) {
    if ((now - last_buzz) >= buzz_period) {
      GPIO_PinState s = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (s==GPIO_PIN_SET)?GPIO_PIN_RESET:GPIO_PIN_SET);
      last_buzz = now;
    }
  } else {
    /* 非响铃时，确保 PB8 在无短鸣时为低 */
    if (!oneshot_until) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
  }

  /* 停止一次性短鸣（若未处于持续响铃） */
  if (!firing && oneshot_until) {
    if ((int32_t)(now - oneshot_until) >= 0) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
      oneshot_until = 0;
    }
  }

  /* 设置态闪烁（500ms） */
  if (setting_mode) {
    static uint32_t last_flash = 0;
    if ((now - last_flash) >= 500) {
      flash_state = !flash_state;
      last_flash = now;
    }
  } else {
    flash_state = 1;
  }

  /* 每秒检查一次闹钟触发（秒变化检测） */
  static uint8_t last_checked_sec = 0xFF;
  RTC_TimeTypeDef sTime; RTC_DateTypeDef sDate;
  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) {
    (void)HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    if (sTime.Seconds != last_checked_sec) {
      last_checked_sec = sTime.Seconds;
      if (!firing && alarm_enabled && sTime.Hours == alarm_hour && sTime.Minutes == alarm_min && sTime.Seconds == 0) {
        firing = 1;
        last_buzz = now;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
      }
    }
  }

  /* 处理按键状态机（消抖 + 每键冷却） */
  /* PA0 状态机 */
  switch (pa0_state) {
    case BTN_IDLE:
      break;
    case BTN_DEBOUNCE_DOWN:
      if ((now - pa0_ts) >= DEBOUNCE_MS) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
          if ((now - pa0_last_action_ts) >= BUTTON_COOLDOWN_MS) {
            pa0_last_action_ts = now;
            if (firing) {
              Clock_StopRinging();
              oneshot_beep(100);
            } else if (!setting_mode) {
              Clock_ToggleEnable();
              oneshot_beep(100);
            } else {
              if (setting_field == 0) setting_field = 1;
              else {
                setting_mode = 0;
                alarm_hour = temp_h; alarm_min = temp_m; alarm_enabled = 1;
                save_bkp();
              }
            }
          }
          pa0_state = BTN_PRESSED;
        } else {
          pa0_state = BTN_IDLE;
        }
      }
      break;
    case BTN_PRESSED:
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
        pa0_state = BTN_DEBOUNCE_UP;
        pa0_ts = now;
      }
      break;
    case BTN_DEBOUNCE_UP:
      if ((now - pa0_ts) >= DEBOUNCE_MS) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) pa0_state = BTN_IDLE;
        else pa0_state = BTN_PRESSED;
      }
      break;
  }

  /* PB1 状态机 */
  switch (pb1_state) {
    case BTN_IDLE:
      break;
    case BTN_DEBOUNCE_DOWN:
      if ((now - pb1_ts) >= DEBOUNCE_MS) {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) {
          if ((now - pb1_last_action_ts) >= BUTTON_COOLDOWN_MS) {
            pb1_last_action_ts = now;
            if (firing) {
              Clock_StopRinging();
              oneshot_beep(100);
            } else if (!setting_mode) {
              RTC_TimeTypeDef sTime; RTC_DateTypeDef sDate;
              if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) {
                (void)HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
                temp_h = sTime.Hours; temp_m = sTime.Minutes;
              } else {
                temp_h = alarm_hour; temp_m = alarm_min;
              }
              setting_mode = 1; setting_field = 0;
              oneshot_beep(100);
            } else {
              if (setting_field == 0) temp_h = (temp_h + 1) % 24;
              else temp_m = (temp_m + 1) % 60;
            }
          }
          pb1_state = BTN_PRESSED;
        } else {
          pb1_state = BTN_IDLE;
        }
      }
      break;
    case BTN_PRESSED:
      if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET) {
        pb1_state = BTN_DEBOUNCE_UP;
        pb1_ts = now;
      }
      break;
    case BTN_DEBOUNCE_UP:
      if ((now - pb1_ts) >= DEBOUNCE_MS) {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET) pb1_state = BTN_IDLE;
        else pb1_state = BTN_PRESSED;
      }
      break;
  }
}

void Clock_SetAlarm(uint8_t hour, uint8_t minute)
{
  alarm_hour = hour % 24;
  alarm_min = minute % 60;
  alarm_enabled = 1;
  save_bkp();
}

void Clock_GetAlarm(uint8_t *enabled, uint8_t *hour, uint8_t *minute)
{
  if (enabled) *enabled = alarm_enabled;
  if (hour) *hour = alarm_hour;
  if (minute) *minute = alarm_min;
}

void Clock_ToggleEnable(void)
{
  alarm_enabled = !alarm_enabled;
  save_bkp();
}

void Clock_StopRinging(void)
{
  firing = 0;
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
}

/* PA0（键1）处理：开/关闹钟；设置时切换字段/保存；响铃时停止响铃 */
void Clock_PA0_Pressed(void)
{
  /* 将按键事件交给周期性状态机处理以防止抖动与重复触发 */
  uint32_t now = HAL_GetTick();
  if (pa0_state == BTN_IDLE) {
    pa0_state = BTN_DEBOUNCE_DOWN;
    pa0_ts = now;
  }
}

/* PB1（键2）处理：进入设置或设置中为当前字段加1；响铃时停止响铃 */
void Clock_PB1_Pressed(void)
{
  /* 将按键事件交给周期性状态机处理以防止抖动与重复触发 */
  uint32_t now = HAL_GetTick();
  if (pb1_state == BTN_IDLE) {
    pb1_state = BTN_DEBOUNCE_DOWN;
    pb1_ts = now;
  }
}

/* 查询接口 */
uint8_t Clock_IsSetting(void)
{
  return setting_mode;
}

void Clock_GetTemp(uint8_t *hour, uint8_t *minute)
{
  if (hour) *hour = temp_h;
  if (minute) *minute = temp_m;
}

uint8_t Clock_GetSettingField(void)
{
  return setting_field;
}

uint8_t Clock_GetFlashState(void)
{
  return flash_state;
}
