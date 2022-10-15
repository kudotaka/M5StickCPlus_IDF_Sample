#pragma once
#include "axp192.h"
#include "freertos/FreeRTOS.h"

#if ( CONFIG_SOFTWARE_BUTTON_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT )
#include "button.h"
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
#include "pcf8563.h"
#endif

#if CONFIG_SOFTWARE_MPU6886_SUPPORT
#include "mpu6886.h"
#endif

#include "driver/gpio.h"
#include "i2c_device.h"

#define PORT_A_SDA_PIN GPIO_NUM_32
#define PORT_A_SCL_PIN GPIO_NUM_33
#define PORT_A_I2C_STANDARD_BAUD 400000
#define PORT_LEVEL_HIGH 1
#define PORT_LEVEL_LOW  0
#define PORT_EXT_PIN_GPIO_NUM_26 GPIO_NUM_26
#define PORT_EXT_PIN_GPIO_NUM_36 GPIO_NUM_36
#define PORT_EXT_PIN_GPIO_NUM_25 GPIO_NUM_25
#define PORT_EXT_PIN_GPIO_NUM_0 GPIO_NUM_0

typedef enum {
    NONE,   //Reset GPIO to default state.
    OUTPUT, //Set GPIO to output mode.
    INPUT,  //Set GPIO to input mode.
    I2C,    //Enable I2C mode. Only available on Port A—GPIO XX(SDA) and Port A—GPIO XX (SCL).
    ADC,    //Enable ADC mode. Only available on Port B—GPIO XX
    DAC,    //Enable DAC mode. Only available on Port B—GPIO XX
    UART    //Enable UART RX/TX mode. UART TX only available on Port 
                    // C—GPIO 14 and UART RX is only available on Port C—GPIO 
                    // 13. Only supports full-duplex UART so setting one pin 
                    // to UART mode will also set the other pin to UART mode.
} pin_mode_t;

#if CONFIG_SOFTWARE_UI_SUPPORT
#include "freertos/semphr.h"
#include "lvgl.h"
#include "disp_driver.h"
#include "disp_spi.h"

extern SemaphoreHandle_t xGuiSemaphore;
#endif

#if ( CONFIG_SOFTWARE_LED_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_LED_SUPPORT )
#include "led.h"
#endif


void M5Stick_Init(void);

#if CONFIG_SOFTWARE_BUTTON_SUPPORT
extern Button_t* button_a;
extern Button_t* button_b;
void M5Stick_Button_Init(void);
#endif

esp_err_t M5Stick_Port_PinMode(gpio_num_t pin, pin_mode_t mode);
bool M5Stick_Port_Read(gpio_num_t pin);
esp_err_t M5Stick_Port_Write(gpio_num_t pin, bool level);
I2CDevice_t M5Stick_Port_A_I2C_Begin(uint8_t device_address, uint32_t baud);
esp_err_t M5Stick_Port_A_I2C_Read(I2CDevice_t device, uint32_t register_address, uint8_t *data, uint16_t length);
esp_err_t M5Stick_Port_A_I2C_Write(I2CDevice_t device, uint32_t register_address, uint8_t *data, uint16_t length);
void M5Stick_Port_A_I2C_Close(I2CDevice_t device);

#if CONFIG_SOFTWARE_UI_SUPPORT
void M5Stick_Display_Init(void);
void M5Stick_Display_SetBrightness(uint8_t brightness);
#endif

void M5Stick_PMU_Init(uint16_t ldo2_volt, uint16_t ldo3_volt, uint16_t dc2_volt, uint16_t dc3_volt);
float M5Stick_PMU_GetBatVolt(void);
float M5Stick_PMU_GetBatCurrent(void);

#if CONFIG_SOFTWARE_LED_SUPPORT
extern Led_t* led_a;
void M5Stick_Led_Init(void);
#endif

#if CONFIG_SOFTWARE_BUZZER_SUPPORT
void M5Stick_Buzzer_Init();
void M5Stick_Buzzer_InitWithPin(gpio_num_t pin);
void M5Stick_Buzzer_Play(void);
void M5Stick_Buzzer_Stop(void);
void M5Stick_Buzzer_Play_Duty(uint32_t duty);
void M5Stick_Buzzer_Play_Duty_Frequency(uint32_t duty, uint32_t frequency);
#endif
