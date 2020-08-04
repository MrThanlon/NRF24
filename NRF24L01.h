
#ifndef NRF24L01_H
#define NRF24L01_H

#include "cstring"
#include "functional"
#include <vector>

class NRF24 {
  std::function<void()> CE_LOW, CE_HIGH, CS_LOW, CS_HIGH;
  std::function<void(uint8_t *TxData, uint8_t *RxData, uint16_t size)>
      SPI_TransmitReceive;
  uint8_t *write_buffer = new uint8_t(33);
  uint8_t *read_buffer = new uint8_t(33);
  std::function<void(uint8_t *, uint16_t)> received_callback;
  std::function<void()> sent_callback, unsent_callback;

public:
  uint8_t *Tx_Address = nullptr;
  uint8_t *Rx_Address = nullptr;

  NRF24(void (*CE_LOW)(), void (*CE_HIGH)(), void (*CS_LOW)(),
        void (*CS_HIGH)(),
        void (*SPI_TransmitReceive)(uint8_t *, uint8_t *, uint16_t));

  bool check();

  void SPI_wrapped(uint8_t *, uint8_t *, uint16_t);

  void write_reg(uint8_t addr, uint8_t *data, uint16_t size, uint8_t check = 3);

  void write_reg(uint8_t addr, uint8_t byte, uint8_t check = 3);

  uint8_t *read_reg(uint8_t addr, uint16_t size);

  uint8_t read_reg(uint8_t addr);

  uint8_t read_stat();

  void init();

  void IRQ_Handler();

  void set_mode_rx();

  void set_mode_tx();

  void set_mode_standby();

  void write_fifo(uint8_t *data, uint16_t size);

  uint16_t read_fifo(uint8_t *&buffer, bool clear = true);

  void transmit(
      uint8_t *data, uint16_t size, void (*resolve)() = []() {},
      void (*reject)() = []() {});

  void transmit(uint8_t *data, uint16_t size);

  void receive(void (*resolve)(uint8_t *, uint16_t));

  void receive();
};

#endif // TEST3_NRF24L01_H
