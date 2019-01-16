#include <stm32f4xx.h>

#include <stdbool.h>
#include <stdint.h>

void SysTick_Handler()
{
	HAL_IncTick();
}

#define OE	GPIO_PIN_7
#define RCLK	GPIO_PIN_8
#define SER	GPIO_PIN_9
#define SRCLK	GPIO_PIN_10
#define SRCLR	GPIO_PIN_11

#define Q_OUT	GPIO_PIN_7

static inline void set(uint16_t pin) {
	HAL_GPIO_WritePin(GPIOD, pin, GPIO_PIN_SET);
}

static inline void reset(uint16_t pin) {
	HAL_GPIO_WritePin(GPIOD, pin, GPIO_PIN_RESET);
}

static inline bool get(int pin) {
	if (pin < 8)
		return HAL_GPIO_ReadPin(GPIOE, 0x100 << pin);
	else if (pin == 8)
		return HAL_GPIO_ReadPin(GPIOE, Q_OUT);
}

static inline uint16_t read() {
	return (GPIOE->IDR & 0xFF00) >> 8;
}

static void fail(int error) {
	while(1) {
		for (int i = 0; i < error; i++) {
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
			HAL_Delay(500);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
			HAL_Delay(500);
		}
		HAL_Delay(2000);
	}
}

static void success() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
	HAL_SuspendTick();
	// Allow debug connection
	DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;
	while(1) {
		__WFI();
	}
}

void setup(void)
{
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
	HAL_InitTick(0);

	// Setup pins
	GPIO_InitTypeDef init = {
		.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11,
		.Mode = GPIO_MODE_OUTPUT_PP,
		.Pull = GPIO_NOPULL,
		.Speed = GPIO_SPEED_FREQ_VERY_HIGH,
	};
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	set(OE);
	reset(SRCLR | SRCLK | RCLK);
	HAL_GPIO_Init(GPIOD, &init);
	// LEDs
	init.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	HAL_GPIO_Init(GPIOD, &init);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
	// Output of IC
	init.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	init.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(GPIOE, &init);
}

static int test() {
	int test = 0;

	// Test outputs
	test = 1;
	set(RCLK);
	reset(OE);
	if (read() != 0)
		goto err;
	reset(RCLK);

	// test shifting one bit
	test = 2;
	set(SRCLR);
	set(SER);
	for (int i = 0; i < 8; i++) {
		set(SRCLK);
		if (read() != 1 << (i-1))
			goto err; // bit should not be propagated anywhere
		if (get(8) != (i == 7))
			goto err; // last bit should be set at last
		set(RCLK);
		if (read() != 1 << i)
			goto err; // bit should be propagated to output
		if (get(8) != (i == 7))
			goto err; // this should not influence last bit out
		reset(SRCLK);
		reset(RCLK);
		reset(SER);
		// everything should be the same
		if (read() != 1 << i)
			goto err;
		if (get(8) != (i == 7))
			goto err;
	}
	test = 4;
	set(SRCLK);
	if (read() != 0x80)
		goto err; // this should not clear output
	if (get(8) != 0)
		goto err; // however, shift out should be cleared
	set(RCLK);
	if (read() != 0 || get(8) != 0)
		goto err; // everything must be cleared

	reset(SRCLK);
	reset(RCLK);
	set(SER);

	if (read() != 0 || get(8) != 0)
		goto err; // nothing should change

	test = 5;

	for (int i = 0; i < 8; i++) {
		set(SRCLK);
		if (read() != (1 << i) - 1)
			goto err; // bit should not be propagated anywhere
		if (get(8) != (i == 7))
			goto err; // last bit should be set at last
		set(RCLK);
		if (read() != (1 << (i + 1)) - 1)
			goto err;
		if (get(8) != (i == 7))
			goto err;
		reset(SRCLK);
		reset(RCLK);
		// everything should be left the same
		if (read() != (1 << (i + 1)) - 1)
			goto err;
		if (get(8) != (i == 7))
			goto err;
	}

	// Test clearing
	test = 6;

	reset(SRCLR);
	if (read() != 0xFF)
		goto err;
	if (get(8) != 0)
		goto err;

	set(RCLK);
	if (read() != 0 || get(8) != 0)
		goto err;

	set(SRCLR);
	if (read() != 0 || get(8) != 0)
		goto err;

	reset(RCLK);
	if (read() != 0 || get(8) != 0)
		goto err;

	// Test propagating bits without output on pins
	test = 7;
	for (int i = 0; i < 8; i++) {
		set(SRCLK);
		if (read() != 0)
			goto err; // bits should not be propagated anywhere
		if (get(8) != (i == 7))
			goto err; // last bit should be set at last
		reset(SRCLK);
		// everything should be left the same
		if (read() != 0)
			goto err;
		if (get(8) != (i == 7))
			goto err;
		HAL_GPIO_TogglePin(GPIOD, SER);
	}

	// Now we'll propagate that to output
	test = 8;
	set(RCLK);
	if (read() != 0xAA || get(8) != 1)
		goto err;
	reset(RCLK);
	if (read() != 0xAA || get(8) != 1)
		goto err;

	return 0;
err:
	// TODO put outputs in failsafe states
	return test;
}

int main(void)
{
	setup();

	int result = test();

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

	if (result != 0)
		fail(result);
	else
		success();

	while (1) { }
	return 0;
}
