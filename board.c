#include "board.h"
#include <stdint.h>

static volatile int g_ms_count;

int millis()
{
    return g_ms_count;
}

void delay_ms(int ms)
{
    int end = g_ms_count + ms;
    while (g_ms_count < end);
}

void SystickHandler()
{
    g_ms_count++;    
}

void board_setup()
{
    SysTick_Config(8000);
    
    // Option byte value: disable VDDA_MONITOR to save some standby current
    if (FLASH->OBR & FLASH_OBR_VDDA_MONITOR)
    {
        FLASH->KEYR = FLASH_OPTKEY1;
        FLASH->KEYR = FLASH_OPTKEY2;
        FLASH->OPTKEYR = FLASH_OPTKEY1;
        FLASH->OPTKEYR = FLASH_OPTKEY2;
        FLASH->CR |= FLASH_CR_OPTER;
        FLASH->CR |= FLASH_CR_STRT;
        delay_ms(10);
        FLASH->CR &= ~FLASH_CR_OPTER;
        
        FLASH->CR |= FLASH_CR_OPTPG;
        OB->USER = 0xDF;
        delay_ms(10);
        FLASH->CR &= ~FLASH_CR_OPTPG;
        
        if (OB->USER == 0x20DF)
        {
            // Note: OBL_LAUNCH causes reboot
            FLASH->CR |= FLASH_CR_OBL_LAUNCH;
        }
    }
    
    // GPIOA    0: Wakeup
    // GPIOA    6: Motor direction, GPIO
    // GPIOA    7: Motor PWM, TIM14, AF4
    // GPIOA 8-11: LEDs, TIM1, AF2
    // GPIOB  4-5: LEDs, TIM3, AF1
    // GPIOB    6: LED, TIM16, AF2
    // GPIOB    7: LED, TIM17, AF2
    
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM14EN | RCC_APB1ENR_PWREN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN;
    
    TIM1->ARR = 1024;
    TIM1->CCMR1 = 0x6060;
    TIM1->CCMR2 = 0x6060;
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E |
                 TIM_CCER_CC1P | TIM_CCER_CC2P | TIM_CCER_CC3P | TIM_CCER_CC4P;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CNT = 0;
    TIM1->CR1 = TIM_CR1_CEN;
    
    TIM3->ARR = 1024;
    TIM3->CCMR1 = 0x6060;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC1P | TIM_CCER_CC2P;
    TIM3->CNT = 256;
    TIM3->CR1 = TIM_CR1_CEN;
    
    TIM14->ARR = 256;
    TIM14->CCMR1 = 0x60;
    TIM14->CCER = TIM_CCER_CC1E;
    TIM14->CCR1 = 0;
    TIM14->CNT = 0;
    TIM14->CR1 = TIM_CR1_CEN;
    
    TIM16->ARR = 1024;
    TIM16->CCMR1 = 0x60;
    TIM16->CCER = TIM_CCER_CC1NE | TIM_CCER_CC1NP;
    TIM16->BDTR |= TIM_BDTR_MOE;
    TIM16->CNT = 512;
    TIM16->CR1 = TIM_CR1_CEN;
    
    TIM17->ARR = 1024;
    TIM17->CCMR1 = 0x60;
    TIM17->CCER = TIM_CCER_CC1NE | TIM_CCER_CC1NP;
    TIM17->BDTR |= TIM_BDTR_MOE;
    TIM17->CNT = 768;
    TIM17->CR1 = TIM_CR1_CEN;
    
    GPIOA->ODR = 0;
    GPIOA->MODER = 0x28AA9000;
//    GPIOA->MODER = 0x28AA0000; // To disable sounds
    GPIOA->AFR[0] = 0x40000000;
    GPIOA->AFR[1] = 0x00002222;

    GPIOB->ODR = 0;
    GPIOB->MODER = 0x0000AA00;
    GPIOB->AFR[0] = 0x22110000;
    
    // Disable wake-up pin to be able to sense the state without the forced
    // pull-down resistor discharging the series capacitor.
    PWR->CSR &= ~PWR_CSR_EWUP1;
}

void board_standby()
{
    __disable_irq();
    GPIOA->MODER = 0x28000000;
    GPIOB->MODER = 0x00000000;
    
    PWR->CR |= PWR_CR_PDDS;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    PWR->CSR |= PWR_CSR_EWUP1;
    PWR->CR |= PWR_CR_CWUF;

    __DSB();
    
    while(1)
    {
        __WFE();
    }
}

extern int main();

/* provided by the linker script */
extern unsigned long _sidata;            /* start address of the static initialization data */
extern unsigned long _sdata;             /* start address of the data section */
extern unsigned long _edata;             /* end address of the data section */
extern unsigned long _sbss;              /* start address of the bss section */
extern unsigned long _ebss;              /* end address of the bss section */
extern unsigned long _estack;

/* Simple static & bss data initializer. */
register void *stack_pointer asm("sp");
void ResetHandler() __attribute__((noreturn, naked));
void ResetHandler() {    
    /* hope that GCC actually places these in registers
     * instead of overwriting them halfway through */
    register unsigned long *src, *dst;
    
    /* copy the data segment into ram */
    src = &_sidata;
    dst = &_sdata;
    if (src != dst)
        while(dst < &_edata)
            *(dst++) = *(src++);
 
    /* zero the bss segment */
    dst = &_sbss;
    while(dst < &_ebss)
        *(dst++) = 0;
    
    board_setup();
    main();
    board_standby();
}

void HardfaultHandler(void)
{
    for(volatile int i = 0; i < 60*8000000; i++);
    board_standby();
}

unsigned int * const g_vector_table[16] __attribute__ ((section("vectors")))= {
    (unsigned int *)    &_estack,
    (unsigned int *)    ResetHandler,
    (unsigned int *)    HardfaultHandler,
    (unsigned int *)    HardfaultHandler,
    [16 + SysTick_IRQn] = (unsigned int *) SystickHandler
};

