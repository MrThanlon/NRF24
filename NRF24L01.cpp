
#include "NRF24L01.h"

/**
 * @brief Compare uint8_t string.
 * @param s1 uint8_t*
 * @param s2 uint8_t*
 * @param size uint16_t*
 * @return bool
 */
bool cmp(const uint8_t *s1, const uint8_t *s2, uint16_t size) {
  for (int i = 0; i < size; i++) {
    if (s1[i] != s2[i]) {
      return false;
    }
  }
  return true;
}

NRF24::NRF24(void (*CE_LOW)(), void (*CE_HIGH)(), void (*CS_LOW)(),
             void (*CS_HIGH)(),
             void (*SPI_TransmitReceive)(uint8_t *, uint8_t *, uint16_t))
    : CE_LOW(CE_LOW), CE_HIGH(CE_HIGH), CS_LOW(CS_LOW), CS_HIGH(CS_HIGH),
      SPI_TransmitReceive(SPI_TransmitReceive) {}

void NRF24::SPI_wrapped(uint8_t *TxData, uint8_t *RxData, uint16_t size) {
  CS_LOW();
  SPI_TransmitReceive(TxData, RxData, size);
  CS_HIGH();
}

/**
 * @brief  Check if module exists.
 * @note   It will write bytes to TX_ADDR.
 * @retval bool
 */
bool NRF24::check() {
  uint8_t test[6] = "\x12\x34\x56\x78\x9a";
  write_reg(0x10, test, 5);
  uint8_t *ans = read_reg(0x10, 5);
  return cmp(ans, test, 5);
}

/**
 * @brief Write bytes to register.
 * @param addr uint8_t
 * @param data uint8_t*
 * @param size uint16_t
 * @param check bool
 */
void NRF24::write_reg(uint8_t addr, uint8_t *data, uint16_t size,
                      uint8_t check) {
  write_buffer[0] = addr | 0x20u;
  memcpy(write_buffer + 1, data, size);
  SPI_wrapped(write_buffer, read_buffer, size + 1);
  for (uint8_t i = 0; i < check; i++) {
    uint8_t *ans = read_reg(addr, size);
    if (cmp(ans, data, size)) {
      return;
    } else {
      SPI_wrapped(write_buffer, read_buffer, size + 1);
    }
  }
}

/**
 * @brief Write a byte to register.
 * @param addr
 * @param byte
 * @param check
 */
void NRF24::write_reg(uint8_t addr, uint8_t byte, uint8_t check) {
  write_buffer[0] = addr | 0x20u;
  write_buffer[1] = byte;
  SPI_wrapped(write_buffer, read_buffer, 2);
  for (uint8_t i = 0; i < check; i++) {
    uint8_t ans = read_reg(addr);
    if (ans == byte) {
      return;
    } else {
      SPI_wrapped(write_buffer, read_buffer, 2);
    }
  }
}

/**
 * @brief Read bytes from register.
 * @param addr
 * @param size
 * @return
 */
uint8_t *NRF24::read_reg(uint8_t addr, uint16_t size) {
  write_buffer[0] = addr;
  SPI_wrapped(write_buffer, read_buffer, size + 1);
  return read_buffer + 1;
}

/**
 * @brief Read a byte from register.
 * @param addr
 * @return
 */
uint8_t NRF24::read_reg(uint8_t addr) {
  write_buffer[0] = addr;
  SPI_wrapped(write_buffer, read_buffer, 2);
  return read_buffer[1];
}

/**
 * @brief Read status register.
 * @return byte
 */
uint8_t NRF24::read_stat() {
  uint8_t nop = 0xff;
  uint8_t ans;
  SPI_wrapped(&nop, &ans, 1);
  return ans;
}

void NRF24::init() {
  CE_LOW();
  CS_HIGH();
  write_reg(0x00, 0x0A);
  write_reg(0x01, 0x03);
  write_reg(0x02, 0x03);
  write_reg(0x04, 0x13);
  write_reg(0x06, 0x26);
  // use pipe-0 to receive ACK
  write_reg(0x0A, Tx_Address, 5);
  // use pipe-1 to receive data
  write_reg(0x0B, Rx_Address, 5);
  write_reg(0x10, Tx_Address, 5);
  write_reg(0x1C, 0x03);
  write_reg(0x1D, 0x06);
}

/**
 * @brief Set RX Mode
 */
void NRF24::set_mode_rx() {
  uint8_t ans = read_reg(0x00);
  ans |= 0b00000001u;
  write_reg(0x00, ans);
  CE_HIGH();
}

/**
 * @brief Set TX Mode
 */
void NRF24::set_mode_tx() {
  uint8_t ans = read_reg(0x00);
  ans &= 0b11111110u;
  write_reg(0x00, ans);
  CE_HIGH();
}

/**
 * @brief Back to Standby-I Mode
 */
void NRF24::set_mode_standby() { CE_LOW(); }

/**
 * @brief Write data to fifo
 * @note It will flush TX_FIFO first
 * @param data uint8_t*
 * @param size uint16_t
 */
void NRF24::write_fifo(uint8_t *data, uint16_t size) {
  // flush
  write_buffer[0] = '\xE1';
  SPI_wrapped(write_buffer, read_buffer, 1);
  // write
  write_buffer[0] = '\xA0';
  memcpy(write_buffer + 1, data, size);
  SPI_wrapped(write_buffer, read_buffer, size + 1);
}

/**
 * @brief Read RX FIFO.
 * FIXME: buffer
 * @note It will clear RX_DR IRQ.
 * @param buffer
 * @param clear bool If clear IRQ after reading.
 * @return length of data
 */
uint16_t NRF24::read_fifo(uint8_t *&buffer, bool clear) {
  write_buffer[0] = '\x60';
  SPI_wrapped(write_buffer, read_buffer, 2);
  uint8_t data_size = read_buffer[1];
  uint8_t stat = read_buffer[0];
  write_buffer[0] = '\x61';
  SPI_wrapped(write_buffer, read_buffer, data_size + 1);
  buffer = read_buffer + 1;
  if (clear) {
    write_reg(0x07, stat & 0b01001111u);
  }
  return data_size;
}

/**
 * @brief Send data.
 * @param data uint8_t*
 * @param size uint16_t
 * @param resolve
 * @param reject
 */
void NRF24::transmit(uint8_t *data, uint16_t size, void (*resolve)(),
                     void (*reject)()) {
  set_mode_standby();
  // set config
  uint8_t stat = read_stat();
  uint8_t config = read_reg(0x00);
  config &= 0b01001110u;
  config |= 0b01000000u;
  write_reg(0x00, config);
  // write TX_FIFO
  write_fifo(data, size);
  // callback
  sent_callback = resolve;
  unsent_callback = reject;
  set_mode_tx();
}

/**
 * Send data, keep last callback.
 * @param data
 * @param size
 */
void NRF24::transmit(uint8_t *data, uint16_t size) {
  set_mode_standby();
  // set config
  uint8_t stat = read_stat();
  uint8_t config = read_reg(0x00);
  config &= 0b01001110u;
  config |= 0b01000000u;
  write_reg(0x00, config);
  // write TX_FIFO
  write_fifo(data, size);
  set_mode_tx();
}

/**
 * @brief Receive data.
 * @note It will call resolve() while received data
 * @param resolve
 */
void NRF24::receive(void (*resolve)(uint8_t *, uint16_t)) {
  set_mode_standby();
  // set config
  uint8_t stat = read_stat();
  uint8_t config = read_reg(0x00);
  config &= 0b00111111u;
  config |= 0b00110001u;
  write_reg(0x00, config);
  // flush fifo
  write_buffer[0] = '\xE2';
  SPI_wrapped(write_buffer, read_buffer, 1);
  // RX mode
  received_callback = resolve;
  set_mode_rx();
}

/**
 * @brief Receive data, keep last callback.
 */
void NRF24::receive() {
  set_mode_standby();
  // set config
  uint8_t stat = read_stat();
  uint8_t config = read_reg(0x00);
  config &= 0b00111111u;
  config |= 0b00110001u;
  write_reg(0x00, config);
  // flush fifo
  write_buffer[0] = '\xE2';
  SPI_wrapped(write_buffer, read_buffer, 1);
  // RX mode
  set_mode_rx();
}

/**
 * @brief IRQ Handling, inject it to extra interrupt.
 */
void NRF24::IRQ_Handler() {
  uint8_t stat = read_stat();
  set_mode_standby();
  if (stat & 0b01000000u) {
    // receive data
    uint16_t size = read_fifo(read_buffer, false);
    // call custom function
    received_callback(read_buffer, size);
    // clear IRQ
    write_reg(0x07, stat & 0b01001111u);
  } else if (stat & 0b00100000u) {
    sent_callback();
    // clear IRQ
    write_reg(0x07, stat & 0b00101111u);
  } else {
    write_buffer[0] = '\xE1';
    SPI_wrapped(write_buffer, read_buffer, 1);
    unsent_callback();
    // clear TX_FIFO
    write_reg(0x07, stat & 0b00011111u);
  }
}