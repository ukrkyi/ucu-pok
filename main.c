#include <stm32f4xx.h>

#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCE_TIME 100

static volatile bool reversed = true;
static volatile bool pwm_down = false;

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
	    HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
		reversed = !reversed;
		pwm_down = !pwm_down;
	}
}

static TIM_HandleTypeDef htim;
static const uint16_t pins[] = {
	[0] = TIM_CHANNEL_1,
	[1] = TIM_CHANNEL_2,
	[2] = TIM_CHANNEL_3,
	[3] = TIM_CHANNEL_4,
};

void setup(void)
{
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();
	GPIO_InitTypeDef conf = {
		.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
		.Mode = GPIO_MODE_AF_PP,
		.Pull = GPIO_NOPULL,
		.Speed = GPIO_SPEED_FREQ_LOW,
		.Alternate = GPIO_AF2_TIM4,
	};
	HAL_GPIO_Init(GPIOD, &conf);
	TIM_Base_InitTypeDef timConf = {
		.Prescaler = 0x0,
		.CounterMode = TIM_COUNTERMODE_UP,
		.Period = 99,
		.ClockDivision = TIM_CLOCKDIVISION_DIV1,
	};
	htim.Instance = TIM4;
	htim.Init = timConf;
	HAL_TIM_PWM_Init(&htim);
	TIM_OC_InitTypeDef pwmConf = {
		.OCMode = TIM_OCMODE_PWM1,
		.Pulse = 0x0,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE,
		.OCIdleState = TIM_OCIDLESTATE_RESET,
	};
	for (int i = 0; i < 4; i++) {
		HAL_TIM_PWM_ConfigChannel(&htim, &pwmConf, pins[i]);
		HAL_TIM_PWM_Start(&htim, pins[i]);
	}
	HAL_InitTick(0);
	conf.Pin = GPIO_PIN_0;
	conf.Mode = GPIO_MODE_IT_RISING;
	conf.Pull = GPIO_PULLDOWN,
	HAL_GPIO_Init(GPIOA, &conf);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

int main(void)
{
	int current = 0;
	setup();
	while (1) {
		int cur_pwm = __HAL_TIM_GET_COMPARE(&htim, pins[current]);
		if (pwm_down) {
			if (cur_pwm == 0) {
				if (reversed)
					current = (current + 3) % 4;
				else
					current = (current + 1) % 4;
				pwm_down = false;
				cur_pwm++; // ?
			} else
				cur_pwm--;
		} else {
			if (cur_pwm == 100) {
				pwm_down = true;
				cur_pwm--; // ?
			} else
				cur_pwm++;
		}
		__HAL_TIM_SET_COMPARE(&htim, pins[current], cur_pwm);
		for (volatile int i = 0; i < 10000; i++);
	}
	return 0;
}
