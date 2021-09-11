
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ws2812_control.h"

//#include "hap.h"

static const char *TAG = "homekitLed";


/*H-S-I*/
extern int led_on;
extern uint16_t led_hue;        //0~360
extern uint16_t led_saturation; //0~100
extern uint16_t led_brightness; //0~100
 

void lightbulb_init(void) //ledc init
{
    ws2812_control_init();
}

static bool hsi2rgb(uint16_t h, uint16_t s, uint16_t i, uint8_t *r, uint8_t *g, uint8_t *b)
{
    bool res = true;
    uint16_t hi, F, P, Q, T;

    if (h > 360) return false;
    if (s > 100) return false;
    if (i > 100) return false;

    hi = (h / 60) % 6;
    F = 100 * h / 60 - 100 * hi;
    P = i * (100 - s) / 100;
    Q = i * (10000 - F * s) / 10000;
    T = i * (10000 - s * (100 - F)) / 10000;

    switch (hi) {
    case 0:
        *r = i;
        *g = T;
        *b = P;
        break;
    case 1:
        *r = Q;
        *g = i;
        *b = P;
        break;
    case 2:
        *r = P;
        *g = i;
        *b = T;
        break;
    case 3:
        *r = P;
        *g = Q;
        *b = i;
        break;
    case 4:
        *r = T;
        *g = P;
        *b = i;
        break;
    case 5:
        *r = i;
        *g = P;
        *b = Q;
        break;
    default:
        return false;
    }
    return res;
}

static void led_update()
{
    uint8_t r,g,b;
    //uint16_t brightness = (led_on==true)?led_brightness:0;
    //ESP_LOGI(TAG,"h:%d,s:%d,i:%d,i2:%d", led_hue,led_saturation,led_brightness,brightness);
    if(!hsi2rgb(led_hue, led_saturation, led_brightness, &r, &g, &b))
    {
        ESP_LOGW(TAG,"r:%d,g:%d,b:%d", r, g, b);
        return;
    }
    r = r*255/100;
    g = g*255/100;
    b = b*255/100;
    ESP_LOGI(TAG,"r:%d,g:%d,b:%d", r, g, b);

    struct led_state new_state;
    for (size_t i = 0; i < NUM_LEDS; i++)
    {
        new_state.leds[i] = g<<16|r<<8|b; //GRB
    }
    
    ws2812_write_leds(new_state);
}

/*LED ON 'service'*/

int led_on = false;
uint16_t _brightness;
int lightbulb_set_on(bool value)
{
    //ESP_LOGI(TAG,"on write %d, bright: %d", (int)value, led_brightness);
    led_on = (int)value;
    if (value) {
        led_brightness = _brightness;
        led_on = true;
    }
    else {
        _brightness = led_brightness;
        led_brightness = 0;
        led_on = false;
    }

    led_update();

    return 0;
}

/*LED hue 'service'*/
uint16_t led_hue;     // hue is scaled 0 to 360
int lightbulb_set_hue(float value)
{
    led_hue = value;
    //ESP_LOGI(TAG,"hue write %d",led_hue);

    if(led_on==true){
        led_update();
    }

    return 0;
}

/*LED saturation 'service'*/
uint16_t led_saturation;     // saturation is scaled 0 to 100
int lightbulb_set_saturation(float value)
{
    led_saturation = value;
    //ESP_LOGI(TAG,"saturation write %d",led_saturation);
    if(led_on){
        led_update();
    }    

    return 0;
}

/*LED brightness 'service'*/
uint16_t led_brightness;     // brightness is scaled 0 to 100
int lightbulb_set_brightness(int value)
{
    led_brightness = value;
    _brightness = led_brightness;
    //ESP_LOGI(TAG,"brightness write %d",led_brightness);

    if(led_on==true){
        led_update();
    }

    return 0;
}

