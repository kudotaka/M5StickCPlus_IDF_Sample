#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "m5stick.h"

#if CONFIG_SOFTWARE_WIFI_SUPPORT
#include "wifi.h"
#endif

#if CONFIG_SOFTWARE_UI_SUPPORT
#include "ui.h"
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
#include "esp_sntp.h"
#endif

#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT )
#include "m5unit.h"
#endif

static const char *TAG = "MY-MAIN";


#if CONFIG_SOFTWARE_BUZZER_SUPPORT
TaskHandle_t xBuzzer;
static void vLoopBuzzerTask(void* pvParameters) {
    ESP_LOGI(TAG, "Buzzer is start!");
    M5Stick_Buzzer_Init();
    uint32_t DUTY_MAX_VALUE = 8190;
    uint32_t duty[] = {DUTY_MAX_VALUE/4*2};
    while (1) {
        vTaskSuspend(NULL);
        for (uint32_t i = 0; i < sizeof(duty)/sizeof(uint32_t); i++)
        {
            ESP_LOGI(TAG, "Buzzer duty:%d", duty[i]);
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 261.6);
            vTaskDelay( pdMS_TO_TICKS(500) );
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 293.665);
            vTaskDelay( pdMS_TO_TICKS(500) );
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 329.63);
            vTaskDelay( pdMS_TO_TICKS(500) );
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 349.228);
            vTaskDelay( pdMS_TO_TICKS(500) );
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 391.995);
            vTaskDelay( pdMS_TO_TICKS(500) );
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 440);
            vTaskDelay( pdMS_TO_TICKS(500) );
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 493.883);
            vTaskDelay( pdMS_TO_TICKS(500) );
            M5Stick_Buzzer_Play_Duty_Frequency(duty[i], 523.251);
            vTaskDelay( pdMS_TO_TICKS(500) );

            M5Stick_Buzzer_Stop();
            vTaskDelay( pdMS_TO_TICKS(5000) );
        }
    }

    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_BUTTON_SUPPORT
TaskHandle_t xButton;
static void button_task(void* pvParameters) {
    ESP_LOGI(TAG, "start button_task");

    while(1){
        if (Button_WasPressed(button_a)) {
            ESP_LOGI(TAG, "BUTTON A PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
        }
        if (Button_WasReleased(button_a)) {
            ESP_LOGI(TAG, "BUTTON A RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }
        if (Button_WasLongPress(button_a, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON A LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
#if CONFIG_SOFTWARE_BUZZER_SUPPORT
            vTaskResume(xBuzzer);
#endif
        }

        if (Button_WasPressed(button_b)) {
            ESP_LOGI(TAG, "BUTTON B PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
        }
        if (Button_WasReleased(button_b)) {
            ESP_LOGI(TAG, "BUTTON B RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }
        if (Button_WasLongPress(button_b, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON B LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
TaskHandle_t xClock;

#if CONFIG_SOFTWARE_WIFI_SUPPORT
TaskHandle_t xRtc;
static bool g_timeInitialized = false;
const char servername[] = "ntp.jst.mfeed.ad.jp";

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    g_timeInitialized = true;
}

void vLoopRtcTask(void *pvParametes)
{
    //PCF8563
    ESP_LOGI(TAG, "start vLoopRtcTask. TaskDelayTime");

    // Set timezone to Japan Standard Time
    setenv("TZ", "JST-9", 1);
    tzset();

    ESP_LOGI(TAG, "ServerName:%s", servername);
    sntp_setservername(0, servername);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    while (1) {
        if (wifi_isConnected() == ESP_OK) {
            sntp_init();
        } else {
            vTaskDelay( pdMS_TO_TICKS(60000) );
            continue;
        }

        ESP_LOGI(TAG, "Waiting for time synchronization with SNTP server");
        while (!g_timeInitialized)
        {
            vTaskDelay( pdMS_TO_TICKS(5000) );
        }

        time_t now = 0;
        struct tm timeinfo = {0};
        time(&now);
        localtime_r(&now, &timeinfo);
        char str1[72] = {0};
        sprintf(str1,"NTP Update : %04d/%02d/%02d %02d:%02d", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
        ESP_LOGI(TAG, "%s", str1);

        rtc_date_t rtcdate;
        rtcdate.year = timeinfo.tm_year+1900;
        rtcdate.month = timeinfo.tm_mon+1;
        rtcdate.day = timeinfo.tm_mday;
        rtcdate.hour = timeinfo.tm_hour;
        rtcdate.minute = timeinfo.tm_min;
        rtcdate.second = timeinfo.tm_sec;
        PCF8563_SetTime(&rtcdate);

        g_timeInitialized = false;
        sntp_stop();

        vTaskDelay( pdMS_TO_TICKS(600000) );
    }
}
#endif

void vLoopClockTask(void *pvParametes)
{
    //PCF8563
    ESP_LOGI(TAG, "start vLoopClockTask.");
 
    // Set timezone to Japan Standard Time
    setenv("TZ", "JST-9", 1);
    tzset();

    while (1) {
        rtc_date_t rtcdate;
        PCF8563_GetTime(&rtcdate);
        char str1[30] = {0};
        sprintf(str1,"%04d/%02d/%02d %02d:%02d:%02d", rtcdate.year, rtcdate.month, rtcdate.day, rtcdate.hour, rtcdate.minute, rtcdate.second);
#if CONFIG_SOFTWARE_UI_SUPPORT
        ui_datetime_set(str1);
#endif

        vTaskDelay( pdMS_TO_TICKS(990) );
    }
}
#endif

#if CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
TaskHandle_t xUnitEnv2;
void vLoopUnitEnv2Task(void *pvParametes)
{
    ESP_LOGI(TAG, "start I2C Sht3x");
    esp_err_t ret = ESP_OK;
    ret = Sht3x_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sht3x_I2CInit Error");
        return;
    }
    ESP_LOGI(TAG, "Sht3x_Init() is OK!");
    while (1) {
        ret = Sht3x_Read();
        if (ret == ESP_OK) {
            vTaskDelay( pdMS_TO_TICKS(100) );
            ESP_LOGI(TAG, "temperature:%f, humidity:%f", Sht3x_GetTemperature(), Sht3x_GetHumidity());
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_temperature_update( Sht3x_GetIntTemperature() );
            ui_humidity_update( Sht3x_GetIntHumidity() );
#endif
        } else {
            ESP_LOGE(TAG, "Sht3x_Read() is error code:%d", ret);
            vTaskDelay( pdMS_TO_TICKS(10000) );
        }

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif

#if CONFIG_SOFTWARE_MPU6886_SUPPORT
TaskHandle_t xMPU6886;
static void mpu6886_task(void* pvParameters) {
    ESP_LOGI(TAG, "start mpu6886_task");

    float ax, ay, az;
    while (1) {
        MPU6886_GetAccelData(&ax, &ay, &az);
        ESP_LOGI(TAG, "MPU6886 Acc x: %.2f, y: %.2f, z: %.2f", ax, ay, az);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_LED_SUPPORT
TaskHandle_t xLED;
static void led_task(void* pvParameters) {
    ESP_LOGI(TAG, "start led_task");

    while(1){
        Led_OnOff(led_a, true);
        vTaskDelay(pdMS_TO_TICKS(1000));

        Led_OnOff(led_a, false);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_SCREEN_DEMO_SUPPORT
TaskHandle_t xSCREEN;
static void screen_task(void* pvParameters) {
    ESP_LOGI(TAG, "start screen_task");

    while(1){
        Axp192_ScreenBreath(0);
        ESP_LOGI(TAG, "Axp192_ScreenBreath (0)");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenBreath(70);
        ESP_LOGI(TAG, "Axp192_ScreenBreath (70)");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenBreath(100);
        ESP_LOGI(TAG, "Axp192_ScreenBreath (100)");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenOnOff(false);
        ESP_LOGI(TAG, "Axp192_ScreenOff");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenOnOff(true);
        ESP_LOGI(TAG, "Axp192_ScreenOn");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_UNIT_LED_SUPPORT
// SELECT GPIO_NUM_XX
TaskHandle_t xExternalLED;
Led_t* led_ext1;
static void external_led_task(void* pvParameters) {
    ESP_LOGI(TAG, "start external_led_task");
    Led_Init();
    if (Led_Enable(GPIO_NUM_26) == ESP_OK) {
        led_ext1 = Led_Attach(GPIO_NUM_26);
    }
    while(1){
        Led_OnOff(led_ext1, true);
        vTaskDelay(pdMS_TO_TICKS(1000));

        Led_OnOff(led_ext1, false);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT
// SELECT GPIO_NUM_XX
TaskHandle_t xExternalButton;
Button_t* button_ext1;
static void external_button_task(void* pvParameters) {
    ESP_LOGI(TAG, "start external_button_task");

    // ONLY M5Stick C Plus
    M5Stick_Port_PinMode(GPIO_NUM_25, INPUT);

    Button_Init();
    if (Button_Enable(GPIO_NUM_36, ACTIVE_HIGH) == ESP_OK) {
        button_ext1 = Button_Attach(GPIO_NUM_36, ACTIVE_HIGH);
    }
    while(1){
        if (Button_WasPressed(button_ext1)) {
            ESP_LOGI(TAG, "BUTTON EXT1 PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
        }
        if (Button_WasReleased(button_ext1)) {
            ESP_LOGI(TAG, "BUTTON EXT1 RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }
        if (Button_WasLongPress(button_ext1, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON EXT1 LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT
TaskHandle_t xExternalRGBLedBlink;
void vexternal_LoopRGBLedBlinkTask(void *pvParametes)
{
    ESP_LOGI(TAG, "start vexternal_LoopRGBLedBlinkTask");

    pixel_settings_t px_ext1;
    uint8_t blink_count = 5;
    uint32_t colors[] = {SK6812_COLOR_BLUE, SK6812_COLOR_LIME, SK6812_COLOR_AQUA
                    , SK6812_COLOR_RED, SK6812_COLOR_MAGENTA, SK6812_COLOR_YELLOW
                    , SK6812_COLOR_WHITE};
    Sk6812_Init(&px_ext1, GPIO_NUM_26, RMT_CHANNEL_0, 1);
    while (1) {
        for (uint8_t c = 0; c < sizeof(colors)/sizeof(uint32_t); c++) {
            for (uint8_t i = 0; i < blink_count; i++) {
                Sk6812_SetAllColor(&px_ext1, colors[c]);
                Sk6812_Show(&px_ext1, RMT_CHANNEL_0);
                vTaskDelay(pdMS_TO_TICKS(1000));

                Sk6812_SetAllColor(&px_ext1, SK6812_COLOR_OFF);
                Sk6812_Show(&px_ext1, RMT_CHANNEL_0);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}
#endif

#if ( CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT || CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT )
TaskHandle_t xDigitDisplay;
DigitDisplay_t* digitdisplay_1;
void vLoopUnitDigitDisplayTask(void *pvParametes)
{
    ESP_LOGI(TAG, "start Digit Display");

// Sample Rowdata
//  --0x01--
// |        |
//0x20     0x02
// |        |
//  --0x40- -
// |        |
//0x10     0x04
// |        |
//  --0x08--   0x80

    uint8_t anime[] = {0x00, 0x30, 0x38, 0x78, 0x79, 0x7f};
    uint8_t animeCurrent = 0;
    uint8_t animeMax = sizeof(anime)/sizeof(uint8_t);
    uint8_t animeDigitPosition = 1;
    uint8_t animeDigitMax = DIGIT_COUNT;
    uint8_t data = 0x00;

    Tm1637_Init();
    if (Tm1637_Enable(GPIO_NUM_33, GPIO_NUM_32) == ESP_OK) {
        digitdisplay_1 = Tm1637_Attach(GPIO_NUM_33, GPIO_NUM_32, BRIGHT_TYPICAL);
    } else {
        ESP_LOGE(TAG, "Digit Display Tm1637_Enable Error");
        vTaskDelete(NULL);
    }
    while(1) {
        Tm1637_ClearDisplay(digitdisplay_1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        for (animeDigitPosition = 1; animeDigitPosition < animeDigitMax+1; animeDigitPosition++) {
            for (animeCurrent = 0; animeCurrent < animeMax; animeCurrent++) {
                for (uint8_t position = DIGIT_COUNT; position > animeDigitPosition-1; position--) {
                    for (uint8_t digit = DIGIT_COUNT; digit > 0; digit--) {
                        data = 0x00;
                        if (digit == position) {
                            data += 0x80;
                        } else {
                            //data = 0x00;
                        }

                        if (digit == animeDigitPosition) {
                            data += anime[animeCurrent];
                        } else if (digit < animeDigitPosition) {
                            data = 0xff;
                        }

                        Tm1637_DisplayBitRowdata(digitdisplay_1, digit-1, data);
                    }
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
        }
    }
    vTaskDelete(NULL); // Should never get to here...

/*
// Sample number
    uint8_t listDisp_1[DIGIT_COUNT];
    uint8_t count = 0;
    Tm1637_Init();
    if (Tm1637_Enable(PORT_D9_PIN, PORT_D10_PIN) == ESP_OK) {
        digitdisplay_1 = Tm1637_Attach(PORT_D9_PIN, PORT_D10_PIN, BRIGHT_TYPICAL);
    } else {
        ESP_LOGE(TAG, "Digit Display Tm1637_Enable Error");
        vTaskDelete(NULL);
    }
    Tm1637_ClearDisplay(digitdisplay_1);
    while(1){
        if (count == UINT8_MAX) {
            count = 0;
        }
        if (count%2) {
            for (uint8_t i = 0; i < DIGIT_COUNT; i++) {
                listDisp_1[i] = i;
            }
        } else {
            for (uint8_t i = 0; i < DIGIT_COUNT; i++) {
                listDisp_1[i] = i+4;
            }
        }
        Tm1637_DisplayAll(digitdisplay_1, listDisp_1);
        count++;
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    vTaskDelete(NULL); // Should never get to here...
*/
/*
// Sample message
    Tm1637_Init();
    if (Tm1637_Enable(PORT_D9_PIN, PORT_D10_PIN) == ESP_OK) {
        digitdisplay_1 = Tm1637_Attach(PORT_D9_PIN, PORT_D10_PIN, BRIGHT_TYPICAL);
    } else {
        ESP_LOGE(TAG, "Digit Display Tm1637_Enable Error");
        vTaskDelete(NULL);
    }
    while (1) {
        Tm1637_ClearDisplay(digitdisplay_1);
        vTaskDelay(pdMS_TO_TICKS(500));
        Tm1637_DisplayStr(digitdisplay_1, "HELL0-1234567890", 500);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelete(NULL); // Should never get to here...
*/
}
#endif

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void app_main()
{
    ESP_LOGI(TAG, "app_main() start.");
    esp_log_level_set("*", ESP_LOG_ERROR);
//    esp_log_level_set("wifi", ESP_LOG_INFO);
//    esp_log_level_set("gpio", ESP_LOG_INFO);
    esp_log_level_set("MY-MAIN", ESP_LOG_INFO);
    esp_log_level_set("MY-UI", ESP_LOG_INFO);
    esp_log_level_set("MY-WIFI", ESP_LOG_INFO);

    M5Stick_Init();

#if CONFIG_SOFTWARE_UI_SUPPORT
    ui_init();
#endif
#if CONFIG_SOFTWARE_WIFI_SUPPORT
    initialise_wifi();
#endif

#if CONFIG_SOFTWARE_BUTTON_SUPPORT
    // BUTTON
    xTaskCreatePinnedToCore(&button_task, "button_task", 4096 * 1, NULL, 2, &xButton, 1);
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
#if CONFIG_SOFTWARE_WIFI_SUPPORT
    // rtc
    xTaskCreatePinnedToCore(&vLoopRtcTask, "rtc_task", 4096 * 1, NULL, 2, &xRtc, 1);
#endif
    // clock
    xTaskCreatePinnedToCore(&vLoopClockTask, "clock_task", 4096 * 1, NULL, 2, &xClock, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
    // UNIT ENV2
    xTaskCreatePinnedToCore(&vLoopUnitEnv2Task, "unit_env2_task", 4096 * 1, NULL, 2, &xUnitEnv2, 1);
#endif

#if CONFIG_SOFTWARE_MPU6886_SUPPORT
    // MPU6886
    xTaskCreatePinnedToCore(&mpu6886_task, "mpu6886_task", 4096 * 1, NULL, 2, &xMPU6886, 1);
#endif

#if CONFIG_SOFTWARE_LED_SUPPORT
    // INTERNAL LED
    xTaskCreatePinnedToCore(&led_task, "led_task", 4096 * 1, NULL, 2, &xLED, 1);
#endif

#if CONFIG_SOFTWARE_SCREEN_DEMO_SUPPORT
    // SCREEN DEMO
    xTaskCreatePinnedToCore(&screen_task, "screen_task", 4096 * 1, NULL, 2, &xSCREEN, 1);
#endif

#if CONFIG_SOFTWARE_BUZZER_SUPPORT
    // BUZZER
    xTaskCreatePinnedToCore(&vLoopBuzzerTask, "buzzer_task", 4096 * 1, NULL, 2, &xBuzzer, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_LED_SUPPORT
    // EXTERNAL LED
    xTaskCreatePinnedToCore(&external_led_task, "external_led_task", 4096 * 1, NULL, 2, &xExternalLED, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT
    // EXTERNAL BUTTON
    xTaskCreatePinnedToCore(&external_button_task, "external_button_task", 4096 * 1, NULL, 2, &xExternalButton, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT
    // EXTERNAL RGB LED BLINK
    xTaskCreatePinnedToCore(&vexternal_LoopRGBLedBlinkTask, "external_rgb_led_blink_task", 4096 * 1, NULL, 2, &xExternalRGBLedBlink, 1);
#endif

#if ( CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT || CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT )
    // DIGIT DISPLAY
    xTaskCreatePinnedToCore(&vLoopUnitDigitDisplayTask, "vLoopUnitDigitDisplayTask", 4096 * 1, NULL, 2, &xDigitDisplay, 1);
#endif
}
