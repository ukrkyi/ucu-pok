#include <stm32f4xx.h>

#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCE_TIME 50

static volatile uint16_t val = 0;

uint32_t lastPress = 0;

void EXTI0_IRQHandler(void)
{
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
	if (HAL_GetTick() - lastPress < DEBOUNCE_TIME)
		return;
	lastPress = HAL_GetTick();
}

void SysTick_Handler()
{
	HAL_IncTick();
	if (HAL_GetTick() - lastPress == DEBOUNCE_TIME &&
	    HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
		val++;
	}
}

void setup(void)
{
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef conf = {
		.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
		.Mode = GPIO_MODE_OUTPUT_PP,
		.Pull = GPIO_NOPULL,
		.Speed = GPIO_SPEED_FREQ_LOW,
		.Alternate = 0,
	};
	HAL_GPIO_Init(GPIOE, &conf);
	HAL_InitTick(0);
	conf.Pin = GPIO_PIN_0;
	conf.Mode = GPIO_MODE_IT_FALLING;
	conf.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &conf);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

int main(void)
{
	setup();
	while (1) {
		HAL_GPIO_WritePin(GPIOE, 0x3FF << 6, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE, (val & 0x3FF) << 6, GPIO_PIN_SET);
		for (volatile int i = 0; i < 10000; i++);
	}
	return 0;
}
