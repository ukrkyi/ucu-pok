#include <stm32f4xx.h>

#include <stdbool.h>
#include <stdint.h>

static volatile bool reversed = true;

void EXTI0_IRQHandler(void)
{
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
	reversed = !reversed;
}

void setup(void)
{
	__HAL_RCC_GPIOD_CLK_ENABLE();
	GPIO_InitTypeDef conf = {
		.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
		.Mode = GPIO_MODE_OUTPUT_PP,
		.Pull = GPIO_PULLDOWN,
		.Speed = GPIO_SPEED_FREQ_LOW,
		.Alternate = 0,
	};
	HAL_GPIO_Init(GPIOD, &conf);
	conf.Pin = GPIO_PIN_0;
	conf.Mode = GPIO_MODE_IT_RISING;
	conf.Pull = GPIO_NOPULL,
	HAL_GPIO_Init(GPIOA, &conf);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

int main(void)
{
	static const uint16_t pins[] = {
		[0] = GPIO_PIN_12,
		[1] = GPIO_PIN_13,
		[2] = GPIO_PIN_14,
		[3] = GPIO_PIN_15,
	};
	int current = 0;
	setup();
	while (1) {
		HAL_GPIO_WritePin(GPIOD, pins[current], GPIO_PIN_RESET);
		if (reversed)
			current = (current + 3) % 4;
		else
			current = (current + 1) % 4;
		HAL_GPIO_WritePin(GPIOD, pins[current], GPIO_PIN_SET);
		for (volatile int i = 0; i < 500000; i++);
	}
	return 0;
}
