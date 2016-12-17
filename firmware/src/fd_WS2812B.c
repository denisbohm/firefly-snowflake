#include "fd_WS2812B.h"

#include "fd_hal_interrupt.h"

#include "fd_nrf.h"

uint32_t fd_WS2812B_pin;

void fd_WS2812B_initialize(uint32_t pin) {
    fd_WS2812B_pin = pin;

    nrf_gpio_pin_clear(fd_WS2812B_pin);
    nrf_gpio_cfg_output(fd_WS2812B_pin);
}

// At 16 MHz each cycle is 62.5 ns.
// Spec of 0.35us +/- 150ns (335 ns to 365 ns)
// - closest we can get: 5 cycles (312.5 ns) [or 6 cycles (375 ns)]
// Spec of 0.9us +/- 150ns (750 ns to 1050 ns)
// - go with: 14 cycles (875 ns)

void fd_WS2812B_grbz(uint32_t grbz, __IO uint32_t *outset, __IO uint32_t *outclr, uint32_t pin_bit) {
    uint32_t i = 24;
__ASM volatile (
    ".syntax unified\n"

    " loop:\n"

    " str %[pin_bit], [%[outset]]\n" // 1 cycle: &NRF_GPIO->OUTSET = bit
    " lsls %[grbz],%[grbz],#1\n" // 1 cycle: shift grbz left, C flag holds bit shifted out
    " bcs code_1\n" // 1 cycle if not taken, 3 cycles 3 if taken

    " code_0:\n"
    " nop\n nop\n"
    " str %[pin_bit], [%[outclr]]\n" // 1 cycle: &NRF_GPIO->OUTCLR = bit
    " nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n" // 9 cycles
    " subs %[i],%[i],#1\n" // 1 cycle: --i
    " bne loop\n" // 3 cycles: if not zero then goto loop
    " b done\n" // 3 cycles: goto done

    " code_1:\n"
    " nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n" // 9 cycles
    " str %[pin_bit], [%[outclr]]\n" // 1 cycle: &NRF_GPIO->OUTCLR = bit
    " subs %[i],%[i],#1\n" // 1 cycle: --i
    " bne loop\n" // 3 cycles: if not zero then goto loop

    " done:\n"
    : // outputs
    : [grbz] "r" (grbz), // inputs
      [outset] "r" (outset),
      [outclr] "r" (outclr),
      [pin_bit] "r" (pin_bit),
      [i] "r" (i)
    : "cc" // clobbers
);
}

void fd_WS2812B_data(const uint32_t *grbzs, uint32_t count) {
    uint32_t pin_bit = 1 << fd_WS2812B_pin;
    const uint32_t *end = grbzs + count;
    while (grbzs < end) {
        uint32_t grbz = *grbzs++;
//        __disable_irq();
        fd_WS2812B_grbz(grbz, &NRF_GPIO->OUTSET, &NRF_GPIO->OUTCLR, pin_bit);
//        __enable_irq();
    }
}

void fd_WS2812B_reset(void) {
    nrf_delay_us(50); // RET code
}