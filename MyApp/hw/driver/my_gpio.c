#include "my_gpio.h"

static GPIO_TypeDef *getPortPtr(uint8_t port)
{
    switch (port) {
    case 0:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        return GPIOA;
    case 1:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        return GPIOB;
    case 2:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        return GPIOC;
    case 3:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        return GPIOD;
    case 4:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        return GPIOE;
    // case 5:
    //     return GPIOF;
    // case 6:
    //     return GPIOG;
    case 7:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        return GPIOH;
    default:
        return NULL;
    }
}

static void gpioExtInit(uint8_t port, uint8_t pin, unsigned long mode)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin);
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(getPortPtr(port), &GPIO_InitStruct);
}

bool gpioExtWrite(uint8_t port, uint8_t pin, uint8_t state)
{
    if (pin > 15)
        return false;

    gpioExtInit(port, pin, GPIO_MODE_OUTPUT_PP);

    GPIO_TypeDef *pPort = getPortPtr(port);
    if (pPort == NULL)
        return false;

    uint16_t pinMask = (1 << pin);
    HAL_GPIO_WritePin(pPort, pinMask, (state > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return true;
}

int8_t gpioExtRead(uint8_t port, uint8_t pin)
{
    if (pin > 15)
        return -1;

    gpioExtInit(port, pin, GPIO_MODE_INPUT);

    GPIO_TypeDef *pPort = getPortPtr(port);
    if (pPort == NULL)
        return -1;

    uint16_t pinMask = (1 << pin);
    return HAL_GPIO_ReadPin(pPort, pinMask) == GPIO_PIN_SET ? 1 : 0;
}