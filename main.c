#include "board.h"
#include <stdbool.h>
#include <stdint.h>

#define LED_INDOORS &TIM3->CCR1
#define LED_BEAM &TIM3->CCR2
#define LED_ROOF &TIM16->CCR1
#define LED_WINDOW &TIM17->CCR1
#define LED_OUTDOORS &TIM1->CCR4

uint32_t rnd_num()
{
    uint32_t rnd = SysTick->VAL ^ millis();
    rnd ^= rnd << 13;
    rnd ^= rnd >> 17;
    rnd ^= rnd << 5;
    return rnd;               
}

int led_gamma(int val, int max)
{
    int dither = millis() % 16;
    return (val * val/max * 1024 + dither * max/16) / max;
}

void fade_on(volatile uint32_t *ccr, int time_ms)
{
    for (int i = 0; i < time_ms; i++)
    {
        *ccr = led_gamma(i, time_ms);
        delay_ms(1);
    }
}

void fade_off(volatile uint32_t *ccr, int time_ms)
{
    for (int i = time_ms - 1; i >= 0; i--)
    {
        *ccr = led_gamma(i, time_ms);
        delay_ms(1);
    }
}

void open_door()
{
    GPIOA->BSRR = (1 << 6);
    TIM14->CCR1 = 0;
    delay_ms(50);
    TIM14->CCR1 = 128;
    delay_ms(50);
    TIM14->CCR1 = 192;
    delay_ms(100);
    TIM14->CCR1 = 0;
    GPIOA->BRR = (1 << 6);
}

void close_door()
{
    GPIOA->BRR = (1 << 6);
    TIM14->CCR1 = 128;
    delay_ms(100);
    TIM14->CCR1 = 64;
    delay_ms(100);
    TIM14->CCR1 = 0;
}

extern const uint8_t _binary_sound_raw_start;
extern const uint8_t _binary_sound_raw_end;
extern const uint8_t _binary_exterminate_raw_start;
extern const uint8_t _binary_exterminate_raw_end;

void play_sound(bool door_open)
{
    if (door_open)
        GPIOA->BSRR = (1 << 6);
    else
        GPIOA->BRR = (1 << 6);

    const uint8_t *p = &_binary_sound_raw_start;
    int len = &_binary_sound_raw_end - p;
    for (int i = 0; i < len; i++)
    {
        if (door_open)
            TIM14->CCR1 = 256 - p[i];
        else
            TIM14->CCR1 = p[i];
        
        // Keep the sample for 4 timer periods, giving samplerate of 7812 Hz.
        for (int i = 0; i < 4; i++)
        {
            while (!(TIM14->SR & TIM_SR_UIF));
            TIM14->SR &= ~TIM_SR_UIF;
        }
        
        // Fade leds in sync
        int led;
        if (i < len / 2)
            led = led_gamma(i, len/2);
        else
            led = led_gamma(len-i, len/2);
        *LED_ROOF = led;
        *LED_BEAM = led;
        *LED_INDOORS = led;
        *LED_WINDOW = led;
    }
    
    TIM14->CCR1 = 0;
    GPIOA->BRR = (1 << 6);
}

void play_gunfire(int time)
{
    int end = millis() + time;
    GPIOA->BSRR = (1 << 6);
    TIM14->CCR1 = 256;
    
    while (millis() < end)
    {
        *LED_OUTDOORS = 0;
        TIM14->CCR1 = 0;
        delay_ms(2);
        TIM14->CCR1 = 256;
        delay_ms(2);
        TIM14->CCR1 = 0;
        delay_ms(2);
        TIM14->CCR1 = 256;
        delay_ms(30);
        *LED_OUTDOORS = 1024;
        delay_ms(20);
    }
    
    TIM14->CCR1 = 0;
    GPIOA->BRR = (1 << 6);
}

void play_exterminate()
{
    GPIOA->BSRR = (1 << 6);
    TIM14->CCR1 = 256;

    const uint8_t *p = &_binary_exterminate_raw_start;
    int len = &_binary_exterminate_raw_end - p;
    for (int i = 0; i < len; i++)
    {
        TIM14->CCR1 = 256 - p[i];
        *LED_OUTDOORS = led_gamma(128 - p[i], 128);
        
        // Keep the sample for 4 timer periods, giving samplerate of 7812 Hz.
        for (int i = 0; i < 4; i++)
        {
            while (!(TIM14->SR & TIM_SR_UIF));
            TIM14->SR &= ~TIM_SR_UIF;
        }
    }
    
    *LED_OUTDOORS = 1024;
    TIM14->CCR1 = 0;
    GPIOA->BRR = (1 << 6);
}

int main(void)
{
    fade_on(LED_OUTDOORS, 2000);
    
    for (int i = 0; i < 500; i++)
    {
        int led = led_gamma(i, 500);
        *LED_ROOF = led;
        *LED_BEAM = led;
        *LED_INDOORS = led;
        *LED_WINDOW = led;
        delay_ms(1);
    }
    
    open_door();
    delay_ms(5000);
    
    int last_on_time = millis();
    bool prev_state = (GPIOA->IDR & 1);
    while (millis() - last_on_time < 5000 && millis() < 120000)
    {
        bool state = (GPIOA->IDR & 1);
        if (state)
        {
            last_on_time = millis();
        }
        
        if (state && !prev_state)
        {
            if ((rnd_num() % 16) == 7)
            {
                // Random effect: Sudden dalek attack
                play_gunfire(700);
                delay_ms(200);
                play_gunfire(1000);
                delay_ms(200);
                play_exterminate();
                play_exterminate();
                delay_ms(200);
                play_gunfire(500);
                close_door();
                play_sound(false);
                play_sound(false);
                *LED_OUTDOORS = 0;
                return 0;
            }
        }
        
        prev_state = state;
    }
    
    for (int i = 0; i < 800; i++)
    {
        int led = led_gamma(800-i, 800);
        *LED_ROOF = led;
        *LED_BEAM = led;
        *LED_INDOORS = led;
        *LED_WINDOW = led;
        delay_ms(1);
    }
    
    play_sound(true);
    delay_ms(200);
    play_sound(true);
    delay_ms(200);
    play_sound(true);
    close_door();
    play_sound(false);
    delay_ms(200);
    play_sound(false);
    
    fade_off(LED_OUTDOORS, 2000);
}

