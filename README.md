# NRF24

## How to use

STM32 HAL Demo

```C++
#include "NRF24L01.h"

NRF24 radio([]() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET); //CE_LOW
}, []() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); // CE_HIGH
}, []() {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // CS_LOW
}, []() {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // CS_HIGH
}, [](uint8_t *TxData, uint8_t *RxData, uint16_t size) {
    HAL_SPI_TransmitReceive(&hspi2, TxData, RxData, size, 0xffff); // SPI Transmit
});
```

Receive data

```C++
// Use callback
radio.receive([](uint8_t *data, uint16_t size) {
  printf("Receive: %s", (char*)data);
  radio.receive();
});
```

Transmit data

```C++
// Use callback
radio.transmit((uint8_t*) "Hello World!\n", 13, []() {
  printf("Data transmission successful.");
}, []() {
  printf("Data transmission failure.");
});
```