/**
 * Copyright (c) @CompanyNameMagicTag 2023-2023. All rights reserved. \n
 *
 * Description: SPI Sample Source. \n
 * Author: @CompanyNameTag \n
 * History: \n
 * 2023-06-25, Create file. \n
 */
#include "led.h"
#include "common_def.h"
#include "dma.h"
#include "gpio.h"
#include "hal_gpio.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "pinctrl.h"
#include "platform_core.h"
#include "soc_osal.h"

#ifndef debug
#define debug
#endif

#ifndef dma_enable
// #define dma_enable
#endif

/**
 * @brief 将原始数据编码为 LED 驱动器可识别的数据
 *
 * @param input_data 原始数据 @ref grb_t
 * @param output_data 编码后的数据 (每个字节包含两个 4-bit 编码信息)
 * @param length  数据长度 (LED 数量) @ref LED_COUNT
 */
void encode_led_data(grb_t *input_data, uint8_t *output_data, uint32_t length) {
  for (uint32_t i = 0; i < length; i++) {
    uint8_t *byte_ptr = (uint8_t *)(&input_data[i]);
    for (uint8_t color = 0; color < LED_DATE_LEN; color++) {
      uint8_t byte = byte_ptr[color];
      for (uint8_t bit_pair = 0; bit_pair < 4; bit_pair++) {
        uint8_t bit_val_1 = (byte >> (7 - 2 * bit_pair)) & 0x01; // 取出高位
        uint8_t bit_val_2 = (byte >> (6 - 2 * bit_pair)) & 0x01; // 取出低位

        uint8_t encoded_val_1 = (bit_val_1 == 0) ? 0b1000 : 0b1100;
        uint8_t encoded_val_2 = (bit_val_2 == 0) ? 0b1000 : 0b1100;

        // 将两个 4-bit 编码信息组合到一个字节
        output_data[i * LED_DATE_LEN * 4 + color * 4 + bit_pair] =
            (encoded_val_1 << 4) | encoded_val_2;
      }
    }
  }
#ifdef debug
  osal_printk("encode_led_data: ");
  for (uint32_t i = 0; i < length * LED_DATE_LEN * 4; i++) {
    osal_printk("0x%02x ", output_data[i]);
  }
  osal_printk("\r\n");
#endif
}

static inline void
app_spi_init_pin(pin_t pin, spi_bus_t bus) { // TODO: 增加防止重复初始化的逻辑
  errcode_t ret;
  // 发送reset信号
  // uapi_pin_set_mode(pin, HAL_PIO_FUNC_GPIO);
  // uapi_gpio_set_dir(pin, GPIO_DIRECTION_OUTPUT);
  // uapi_gpio_set_val(pin, GPIO_LEVEL_LOW);
  // osal_udelay(100);
#ifdef debug
  osal_printk("spi%d pinmux start!\r\n", bus); // TODO:debug
#endif
  switch (bus) {
  case SPI_BUS_0:
    ret = uapi_pin_set_mode(pin, HAL_PIO_SPI0_TXD);
    break;
  case SPI_BUS_1:
    ret = uapi_pin_set_mode(pin, HAL_PIO_SPI1_TXD);
    break;
  case SPI_BUS_2:
    ret = uapi_pin_set_mode(pin, HAL_PIO_SPI2_TXD);
    break;
  default:
    osal_printk("spi bus error!\r\n");
    ret = ERRCODE_DMA_REG_ADDR_INVALID;
    break;
  }
  if (ret != ERRCODE_SUCC) {
    osal_printk("set pin mode failed with errcode = %d\n", ret);
  }
}

static inline void __attribute__((always_inline))
app_spi_master_init_config(spi_bus_t bus) {
#ifdef debug
  osal_printk("spi%d master init start!\r\n", bus); // TODO:debug
#endif
  spi_attr_t config = {0};
  spi_extra_attr_t ext_config = {0};

#define SPI_SLAVE_NUM 1
#define SPI_FREQUENCY 3
#define SPI_CLK_POLARITY 0
#define SPI_CLK_PHASE 0
#define SPI_FRAME_FORMAT 0
#define SPI_FRAME_FORMAT_STANDARD 0
#define SPI_FRAME_SIZE_8 0x1f
#define SPI_TMOD 0
#define SPI_WAIT_CYCLES 0x10

  config.is_slave = false;
  config.slave_num = SPI_SLAVE_NUM;
  config.bus_clk = SPI_CLK_FREQ;
  config.freq_mhz = SPI_FREQUENCY;
  config.clk_polarity = SPI_CLK_POLARITY;
  config.clk_phase = SPI_CLK_PHASE;
  config.frame_format = SPI_FRAME_FORMAT;
  config.spi_frame_format = HAL_SPI_FRAME_FORMAT_STANDARD;
  config.frame_size = SPI_FRAME_SIZE_8;
  config.tmod = SPI_TMOD;
  config.sste = 1;

  errcode_t ret;
#ifdef dma_enable
  ext_config.tx_use_dma = true;
#ifdef debug
  osal_printk("spi%d master dma init start!\r\n", bus); // TODO:debug
#endif
  ret = uapi_dma_init();
  if (ret != ERRCODE_SUCC) {
    osal_printk("uapi_dma_init failed .\n");
  }
#ifdef debug
  osal_printk("spi%d master dma init end!\r\n", bus);   // TODO:debug
  osal_printk("spi%d master dma open start!\r\n", bus); // TODO:debug
#endif
  ret = uapi_dma_open();
  if (ret != ERRCODE_SUCC) {
    osal_printk("uapi_dma_init failed .\n");
  }
#ifdef debug
  osal_printk("spi%d master dma open end!\r\n", bus); // TODO:debug
#endif
#endif
  ret = uapi_spi_init(bus, &config, &ext_config);
  if (ret != ERRCODE_SUCC) {
    osal_printk("uapi_spi_init failed .\n");
    osal_printk("errcode = %d\n", ret);
  } else {
    osal_printk("uapi_spi_init success .\n");
  }
#ifdef debug
  osal_printk("spi%d master init end!\r\n", bus); // TODO:debug
#endif
}

/**
 * @brief 处理用于LED数据传输的SPI通信的任务函数。
 *
 * 该函数初始化SPI引脚和SPI主机配置，编码LED数据，并通过SPI循环发送，直到传输成功。
 *
 * @param arg 指向LED数据和配置结构的指针 (led_data_p)。
 * @return 当任务完成时返回NULL。
 */
void *spi_led_transfer_task(led_data_p arg) {
  if (arg != NULL) {
    osal_printk("spi_master_task arg is not NULL\n");
  } else {
    osal_printk("spi_master_task arg is NULL\nspi_master_task exit\n");
    return NULL;
  }

  /* SPI master init config. */
  app_spi_master_init_config(arg->bus);

  int SPI_TRANSFER_LEN = arg->length * LED_DATE_LEN * 4;

  /* SPI data config. */
  uint8_t *tx_data = osal_kmalloc(SPI_TRANSFER_LEN, (unsigned int)NULL);

  encode_led_data(arg->led_data, tx_data, arg->length);

  spi_xfer_data_t data = {
      .tx_buff = tx_data,
      .tx_bytes = SPI_TRANSFER_LEN,
  };
  // 复位信号
  uint8_t tx_reset_data[80] = {0};
  spi_xfer_data_t reset_data = {
      .tx_buff = tx_reset_data,
      .tx_bytes = 80,
  };

  while (1) {
    osal_printk("spi%d master send start!\r\n", arg->bus);
    /* SPI pinmux. */
    app_spi_init_pin(arg->pin, arg->bus);
    uapi_spi_master_write(arg->bus, &reset_data, 0xFFFFFFFF);
    if (uapi_spi_master_write(arg->bus, &data, 0xFFFFFFFF) == ERRCODE_SUCC) {
      osal_printk("spi%d master send succ!\r\n", arg->bus);
      uapi_pin_set_mode(arg->pin, HAL_PIO_FUNC_GPIO);
      uapi_gpio_set_dir(arg->pin, GPIO_DIRECTION_OUTPUT);
      uapi_gpio_set_val(arg->pin, GPIO_LEVEL_LOW);
      break;
    } else {
#ifdef debug
      osal_printk("spi%d master send failed!\r\n", arg->bus);
#endif
      continue;
    }
  }
  osal_kfree(tx_data);
  return NULL;
}