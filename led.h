#ifndef LED_H
#define LED_H

#include "spi.h"
#include <stdint.h>

/**
 * @brief LED 驱动器数据结构
 *
 */
typedef struct {
  uint8_t green;
  uint8_t red;
  uint8_t blue;
} grb_t, *grb_p;

/**
 * @brief LED 驱动数据结构
 *
 * @parma (grb_t*)led_data LED 数据
 * @parma (uint8_t)length 数据长度 (LED 数量)
 * @parma (pin_t)pin 引脚 @ref pin_t @note 仅支持 SPI 引脚
 * @parma (spi_bus_t)bus SPI 总线 @ref spi_bus_t @note 仅支持 SPI 总线
 *
 * @note spi驱动led, 通过SPI总线发送数据到LED驱动器,
 */
typedef struct {
  grb_p led_data;
  uint8_t length;
  pin_t pin;
  spi_bus_t bus;
} led_data_t, *led_data_p;

#define LED_DATE_LEN 3 ///< 一个 LED 的数据长度（即3个字节，GRB顺序）

/**
 * @brief 处理用于LED数据传输的SPI通信的任务函数。
 *
 * 该函数初始化SPI引脚和SPI主机配置，编码LED数据，并通过SPI循环发送，直到传输成功。
 *
 * @param arg 指向LED数据和配置结构的指针 (led_data_p)。
 * @return 当任务完成时返回NULL。
 */
void *spi_led_transfer_task(led_data_p arg);

#endif