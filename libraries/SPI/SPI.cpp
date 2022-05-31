/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@arduino.cc>
 * Copyright (c) 2014 by Paul Stoffregen <paul@pjrc.com> (Transaction API)
 * Copyright (c) 2014 by Matthijs Kooijman <matthijs@stdin.nl> (SPISettings AVR)
 * Copyright (c) 2014 by Andrew J. Kroll <xxxajk@gmail.com> (atomicity fixes)
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "SPI.h"


uint8_t ArduinoSPI::initialized = 0;
uint8_t ArduinoSPI::interruptMode = 0;
uint8_t ArduinoSPI::interruptMask = 0;
uint8_t ArduinoSPI::interruptSave = 0;

ArduinoSPI::ArduinoSPI(spi_ctrl_t *g_spi_ctrl
                      ,const spi_cfg_t *g_spi_cfg
                      ,const spi_extended_cfg_t *g_spi_ext_cfg):
  _g_spi_ctrl(g_spi_ctrl)
, _g_spi_cfg(g_spi_cfg)
, _g_spi_ext_cfg(g_spi_ext_cfg)
, _g_spi_callback_event(SPI_EVENT_TRANSFER_ABORTED)
, _clk_phase(SPI_CLK_PHASE_EDGE_ODD)
, _clk_polarity(SPI_CLK_POLARITY_LOW)
, _bit_order(SPI_BIT_ORDER_MSB_FIRST)
{
}

ArduinoSPI::ArduinoSPI(spi_ctrl_t *g_spi_ctrl
                      ,const spi_cfg_t *g_spi_cfg
                      /*,const sci_spi_extended_cfg_t *g_spi_ext_cfg*/):
  _g_spi_ctrl(g_spi_ctrl)
, _g_spi_cfg(g_spi_cfg)
//, _g_sci_spi_ext_cfg(g_spi_ext_cfg)
, _g_spi_callback_event(SPI_EVENT_TRANSFER_ABORTED)
, _clk_phase(SPI_CLK_PHASE_EDGE_ODD)
, _clk_polarity(SPI_CLK_POLARITY_LOW)
, _bit_order(SPI_BIT_ORDER_MSB_FIRST)
{
}


void ArduinoSPI::begin()
{
  if(!initialized){
      R_SPI_Open(_g_spi_ctrl, _g_spi_cfg);
      initialized = true;
  }
}

void ArduinoSPI::end() {
  if (initialized){
      initialized = false;
  }
}

void ArduinoSPI::usingInterrupt(int interruptNumber)
{
}

void ArduinoSPI::notUsingInterrupt(int interruptNumber)
{
}

uint8_t ArduinoSPI::transfer(uint8_t data) {
  uint8_t rxbuf;
  R_SPI_WriteRead(_g_spi_ctrl, &data, &rxbuf, 1, SPI_BIT_WIDTH_8_BITS);
  return rxbuf;
}

uint16_t ArduinoSPI::transfer16(uint16_t data) {
  uint16_t rxbuf;
  R_SPI_WriteRead(_g_spi_ctrl, &data, &rxbuf, 1, SPI_BIT_WIDTH_16_BITS);
  return rxbuf;
}

void ArduinoSPI::transfer(void *buf, size_t count) {
  R_SPI_WriteRead(_g_spi_ctrl, buf, buf, count, SPI_BIT_WIDTH_8_BITS);
}

void ArduinoSPI::beginTransaction(arduino::SPISettings settings) {
  // data mode
  switch(settings.getDataMode()){
      case arduino::SPI_MODE0:
          _clk_polarity = SPI_CLK_POLARITY_LOW;
          _clk_phase = SPI_CLK_PHASE_EDGE_ODD;
          break;
      case arduino::SPI_MODE1:
          _clk_polarity = SPI_CLK_POLARITY_LOW;
          _clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
          break;
      case arduino::SPI_MODE2:
          _clk_polarity = SPI_CLK_POLARITY_HIGH;
          _clk_phase = SPI_CLK_PHASE_EDGE_ODD;
          break;
      case arduino::SPI_MODE3:
          _clk_polarity = SPI_CLK_POLARITY_HIGH;
          _clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
          break;
  }
  // bit order
  if(settings.getBitOrder() == LSBFIRST){
      _bit_order = SPI_BIT_ORDER_LSB_FIRST;
  } else {
      _bit_order = SPI_BIT_ORDER_MSB_FIRST;
  }

  if(initialized){
      R_SPI_Close(_g_spi_ctrl);
  }

  // Clock settings
  rspck_div_setting_t spck_div = _g_spi_ext_cfg->spck_div;
  R_SPI_CalculateBitrate(settings.getClockFreq(), &spck_div);

  R_SPI_Open(_g_spi_ctrl, _g_spi_cfg);

  spi_instance_ctrl_t * p_ctrl = (spi_instance_ctrl_t *)_g_spi_ctrl;
  uint32_t spcmd0 = p_ctrl->p_regs->SPCMD[0];
  uint32_t spbr = p_ctrl->p_regs->SPBR;

  /* Configure CPHA setting. */
  spcmd0 |= (uint32_t) _clk_phase;

  /* Configure CPOL setting. */
  spcmd0 |= (uint32_t) _clk_polarity << 1;

  /* Configure Bit Order (MSB,LSB) */
  spcmd0 |= (uint32_t) _bit_order << 12;

  /* Configure the Bit Rate Division Setting */
  spcmd0 |= (uint32_t) spck_div.brdv << 2;

  p_ctrl->p_regs->SPCMD[0] = (uint16_t) spcmd0;
  p_ctrl->p_regs->SPBR = (uint8_t) spck_div.spbr;

  // Update settings
  spcmd0 = p_ctrl->p_regs->SPCMD[0];
  spbr = p_ctrl->p_regs->SPBR;
}

void ArduinoSPI::endTransaction(void) {

}

void ArduinoSPI::attachInterrupt() {

}

void ArduinoSPI::detachInterrupt() {

}


#if SPI_HOWMANY > 0
ArduinoSPI SPI(&g_spi0_ctrl, &g_spi0_cfg, &g_spi0_ext_cfg);
void spi_callback(spi_callback_args_t *p_args)
{
    if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
    }
}
#endif
#if SPI_HOWMANY > 1
ArduinoSPI SPI1(&g_spi1_ctrl, &g_spi1_cfg/*, &g_spi1_cfg_extend*/);
void spi1_callback(spi_callback_args_t *p_args)
{
    if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
    }
}
#endif