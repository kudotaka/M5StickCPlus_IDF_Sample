#include "stdbool.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_common.h"
#include "esp_idf_version.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "m5stick.h"

#if CONFIG_SOFTWARE_UI_SUPPORT
#include "lvgl.h"
#include "lvgl_helpers.h"
#endif

#if CONFIG_SOFTWARE_BUZZER_SUPPORT
#include "driver/ledc.h"
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

#endif

static const char *TAG = "M5StickCPlus";

void M5Stick_Init(void) {
ESP_LOGI(TAG, "M5Stick_Init Init().");

#if CONFIG_SOFTWARE_UI_SUPPORT
    M5Stick_PMU_Init(3300, 0, 0, 2700);
    M5Stick_Display_Init();
#else
    M5Stick_PMU_Init(0, 0, 0, 0);
#endif

#if CONFIG_SOFTWARE_BUTTON_SUPPORT
    M5Stick_Button_Init();
#endif

#if CONFIG_SOFTWARE_LED_SUPPORT
    M5Stick_Led_Init();
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
    PCF8563_Init();
#endif

#if CONFIG_SOFTWARE_MPU6886_SUPPORT
    MPU6886_Init();
#endif

}

/* ===================================================================================================*/
/* --------------------------------------------- BUTTON ----------------------------------------------*/
#if CONFIG_SOFTWARE_BUTTON_SUPPORT
Button_t* button_a;
Button_t* button_b;

void M5Stick_Button_Init(void) {
    Button_Init();
    if (Button_Enable(GPIO_NUM_37) == ESP_OK) {
        button_a = Button_Attach(GPIO_NUM_37);
    }
    if (Button_Enable(GPIO_NUM_39) == ESP_OK) {
        button_b = Button_Attach(GPIO_NUM_39);
    }
}
#endif
/* ----------------------------------------------- End -----------------------------------------------*/
/* ===================================================================================================*/

/* ===================================================================================================*/
/* ----------------------------------------- Expansion Ports -----------------------------------------*/
static esp_err_t check_pins(gpio_num_t pin, pin_mode_t mode){
    esp_err_t err = ESP_ERR_NOT_SUPPORTED;
/*    if (pin != PORT_A_SDA_PIN && pin != PORT_A_SCL_PIN)
    {
        ESP_LOGE(TAG, "Only Port A (GPIO 26 and 32) are supported. Pin selected: %d", pin);
    }
    else if (mode == I2C && (pin != PORT_A_SDA_PIN && pin != PORT_A_SCL_PIN))
    {
        ESP_LOGE(TAG, "I2C is only supported on GPIO 26 (SDA) and GPIO 32 (SCL).");
    }
    else 
    {
*/
        err = ESP_OK;
/*    }
*/
    return err;
}

esp_err_t M5Stick_Port_PinMode(gpio_num_t pin, pin_mode_t mode){
    esp_err_t err = check_pins(pin, mode);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Invalid mode selected for GPIO %d. Error code: 0x%x.", pin, err);
        return err;
    }

    if (mode == OUTPUT || mode == INPUT){
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        io_conf.pin_bit_mask = (1ULL << pin);

        if (mode == OUTPUT){
            io_conf.mode = GPIO_MODE_OUTPUT; 
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            err = gpio_config(&io_conf);
            if (err != ESP_OK){
                ESP_LOGE(TAG, "Error configuring GPIO %d. Error code: 0x%x.", pin, err);
            }
        } else{
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            err = gpio_config(&io_conf);
            if (err != ESP_OK){
                ESP_LOGE(TAG, "Error configuring GPIO %d. Error code: 0x%x.", pin, err);
            }
        }  
    }

    return err;
}

bool M5Stick_Port_Read(gpio_num_t pin){
    ESP_ERROR_CHECK_WITHOUT_ABORT(check_pins(pin, INPUT));

    return gpio_get_level(pin);
}

esp_err_t M5Stick_Port_Write(gpio_num_t pin, bool level){
    esp_err_t err = check_pins(pin, OUTPUT);
    
    err = gpio_set_level(pin, level);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error setting GPIO %d state. Error code: 0x%x.", pin, err);
    }
    return err;
}

I2CDevice_t M5Stick_Port_A_I2C_Begin(uint8_t device_address, uint32_t baud){
    return i2c_malloc_device(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, baud < 100000 ? baud : PORT_A_I2C_STANDARD_BAUD, device_address);
}

esp_err_t M5Stick_Port_A_I2C_Read(I2CDevice_t device, uint32_t register_address, uint8_t *data, uint16_t length){
    return i2c_read_bytes(device, register_address, data, length);
}

esp_err_t M5Stick_Port_A_I2C_Write(I2CDevice_t device, uint32_t register_address, uint8_t *data, uint16_t length){
    return i2c_write_bytes(device, register_address, data, length);
}

void M5Stick_Port_A_I2C_Close(I2CDevice_t device){
    i2c_free_device(device);
}

/* ----------------------------------------------- End -----------------------------------------------*/
/* ===================================================================================================*/

/* ==================================================================================================*/
/* ---------------------------------------------- PMU -----------------------------------------------*/
float M5Stick_PMU_GetBatVolt(void) {
    return Axp192_GetBatVolt();
}

float M5Stick_PMU_GetBatCurrent(void) {
    return Axp192_GetBatCurrent();
}

void M5Stick_PMU_SetPowerIn(uint8_t mode) {
    if (mode) {
        Axp192_SetGPIO0Mode(0);
        Axp192_EnableExten(0);
    } else {
        Axp192_EnableExten(1);
        Axp192_SetGPIO0Mode(1);
    }
}

void M5Stick_PMU_Init(uint16_t ldo2_volt, uint16_t ldo3_volt, uint16_t dc2_volt, uint16_t dc3_volt) {
    uint8_t value = 0x00;
    value |= (ldo2_volt > 0) << AXP192_LDO2_EN_BIT;
    value |= (ldo3_volt > 0) << AXP192_LDO3_EN_BIT;
    value |= (dc2_volt > 0) << AXP192_DC2_EN_BIT;
    value |= (dc3_volt > 0) << AXP192_DC3_EN_BIT;
    value |= 0x01 << AXP192_DC1_EN_BIT;

    Axp192_Init();

    // value |= 0x01 << AXP192_EXT_EN_BIT;
    Axp192_SetLDO23Volt(ldo2_volt, ldo3_volt);
    // Axp192_SetDCDC1Volt(3300);
    Axp192_SetDCDC2Volt(dc2_volt);
    Axp192_SetDCDC3Volt(dc3_volt);
    Axp192_SetVoffVolt(3000);
    Axp192_SetChargeCurrent(CHARGE_Current_100mA);
    Axp192_SetChargeVoltage(CHARGE_VOLT_4200mV);
    Axp192_EnableCharge(1);
    Axp192_SetPressStartupTime(STARTUP_128mS);
    Axp192_SetPressPoweroffTime(POWEROFF_4S);
    Axp192_EnableLDODCExt(value);
    Axp192_SetGPIO4Mode(1);
    Axp192_SetGPIO2Mode(1);
    Axp192_SetGPIO2Level(0);

    Axp192_SetGPIO0Volt(3300);
    Axp192_SetAdc1Enable(0xfe);
    Axp192_SetGPIO1Mode(1);
    M5Stick_PMU_SetPowerIn(0);
}
/* ----------------------------------------------- End -----------------------------------------------*/
/* ===================================================================================================*/

/* ===================================================================================================*/
/* --------------------------------------------- DISPLAY ---------------------------------------------*/
#if CONFIG_SOFTWARE_UI_SUPPORT

#define LV_TICK_PERIOD_MS 1

SemaphoreHandle_t xGuiSemaphore;

static void guiTask(void *pvParameter);
static void lv_tick_task(void *arg);

void M5Stick_Display_Init(void) {
    xGuiSemaphore = xSemaphoreCreateMutex();

    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    lv_init();

    // Initialize SPI or I2C bus used by the drivers
    lvgl_driver_init();

    lv_color_t* buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t* buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif

    static lv_disp_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820         \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A    \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D     \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

    // Actual size in pixels, not bytes.
    size_in_px *= 8;
#endif

    // Initialize the working buffer depending on the selected display
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    xSemaphoreGive(xGuiSemaphore);

    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 2, NULL, 1);
}

static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void guiTask(void *pvParameter) {
    
    (void) pvParameter;

    while (1) {
        // Delay 1 tick (assumes FreeRTOS tick is 10ms
        vTaskDelay(pdMS_TO_TICKS(10));

        // Try to take the semaphore, call lvgl related function on success
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }

    // A task should NEVER return
    vTaskDelete(NULL);
}

void M5Stick_Display_SetBrightness(uint8_t brightness) {
    Axp192_ScreenBreath(brightness);
}
#endif
/* ----------------------------------------------- End -----------------------------------------------*/
/* ===================================================================================================*/

/* ===================================================================================================*/
/* --------------------------------------------- LED ----------------------------------------------*/
#if CONFIG_SOFTWARE_LED_SUPPORT
Led_t* led_a;

void M5Stick_Led_Init(void) {
    Led_Init();
    if (Led_Enable(GPIO_NUM_10) == ESP_OK) {
        led_a = Led_Attach(GPIO_NUM_10);
    }
}
#endif
/* ----------------------------------------------- End -----------------------------------------------*/
/* ===================================================================================================*/

/* ===================================================================================================*/
/* --------------------------------------------- BUZZER ----------------------------------------------*/
#if CONFIG_SOFTWARE_BUZZER_SUPPORT
void M5Stick_Buzzer_Init() {
    M5Stick_Buzzer_InitWithPin(GPIO_NUM_2);
}

void M5Stick_Buzzer_InitWithPin(gpio_num_t pin) {
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void M5Stick_Buzzer_Play() {
    M5Stick_Buzzer_Play_Duty(LEDC_DUTY);
}

void M5Stick_Buzzer_Stop() {
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0));
}

void M5Stick_Buzzer_Play_Duty(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void M5Stick_Buzzer_Play_Duty_Frequency(uint32_t duty, uint32_t frequency) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

#endif
/* ----------------------------------------------- End -----------------------------------------------*/
/* ===================================================================================================*/
