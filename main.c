#include <stdio.h>
#include <inttypes.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"

#include "inc_decoder_pulse.pio.h"
#include "incremental_mpg.pio.h"

#include "mpg_config.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

#ifndef PICO_DEFAULT_LED_PIN
#warning blink_simple example requires a board with a regular LED
#endif


void inc_decoder_pulse_program_init(PIO pio, uint sm, uint offset, uint pin) {
    // 2 input pins
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 2, false);
    pio_sm_config c = inc_decoder_pulse_program_get_default_config(offset);
    sm_config_set_in_pin_base(&c, pin);
    sm_config_set_in_pin_count(&c, 2);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_NONE);
    // shift to left, autopull disabled
    sm_config_set_in_shift(&c, false, false, 32);
    sm_config_set_out_shift(&c, true, false, 32);
    // sm_config_set_set_pins(&c, pin, 1);
    sm_config_set_clkdiv(&c, 15.0);
    pio_sm_init(pio, sm, offset, &c);
}

void incremental_mpg_program_init(PIO pio, uint sm, uint offset, uint pin) {
    // 2 output pins
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 2, true);
    pio_sm_config c = incremental_mpg_program_get_default_config(offset);
    sm_config_set_set_pin_base(&c, pin);
    sm_config_set_set_pin_count(&c, 2);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_NONE);
    // shift to left, autopull disabled
    sm_config_set_in_shift(&c, false, false, 32);
    sm_config_set_out_shift(&c, true, false, 32);
    sm_config_set_set_pins(&c, pin, 2);
    // The DM542T requires a 5us DIR setup time
    // TMC2100 can deal with ~20ns
    sm_config_set_clkdiv(&c, 15.0);
    pio_sm_init(pio, sm, offset, &c);
}

// Initialize the GPIO for the LED
void pico_led_init(void) {
#ifdef PICO_DEFAULT_LED_PIN
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#else
#error need LED pin
#endif
}

// Turn the LED on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#endif
}

PIO pio[2] = {NULL, NULL};
uint sm[2];
uint offset[2];
int dma[1];

static void init_dma(PIO p0, uint sm0, PIO p1, uint sm1) {

    dma[0] = dma_claim_unused_channel(0);
    if (dma[0] < 0) {
        while (1) {
            printf("ERR: failed to allocate DMA channel\n");
        }
    }
    // int dma1 = dma_claim_unused_channel(true);

    dma_channel_config c0 = dma_channel_get_default_config(dma[0]);
    channel_config_set_read_increment(&c0, false);
    channel_config_set_write_increment(&c0, false);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);

    // dma_channel_config c1 = dma_channel_get_default_config(1);
    // channel_config_set_read_increment(&c1, false);
    // channel_config_set_write_increment(&c1, false);
    channel_config_set_dreq(&c0, pio_get_dreq(p0, sm0, false));
    // channel_config_set_dreq(&c1, pio_get_dreq(pio0, sm0, false));
    channel_config_set_chain_to(&c0, dma[0]);
    // channel_config_set_chain_to(&c1, dma0);
    channel_config_set_irq_quiet(&c0, true);
    // channel_config_set_irq_quiet(&c1, true);

    // the RP2350 has an "ENDLESS" DMA mode so that a single DMA channel can be used for continous transfers, whereas one had to chain two DMA channels on the RP2040.
    dma_channel_configure(
        dma[0], 
        &c0,
        &p1->txf[sm1],        // Destination pointer
        &p0->rxf[sm0],      // Source pointer
        0xF0000000 | 4, // endless via high 4 bits being 0xF
        true                // Start immediately
    );
    // dma_channel_configure(dma1, &c1,
    //     &p1->txf[sm1],        // Destination pointer
    //     &p0->rxf[sm0],      // Source pointer
    //     16, // Number of transfers
    //     false                // Start immediately
    // );

    // dma_channel_start(dma[0]);
    // dma_channel_start(dma1);
}

int main() {
    stdio_init_all();
    pico_led_init();

    // gpio_set_function(PIO_INA_GPIO, GPIO_FUNC_PIO0);
    // gpio_set_function(PIO_INB_GPIO, GPIO_FUNC_PIO0);
    // gpio_set_dir(PIO_INA_GPIO, GPIO_OUT);
    // gpio_set_dir(PIO_INB_GPIO, GPIO_OUT);

    bool rc0 = pio_claim_free_sm_and_add_program(&inc_decoder_pulse_program, &pio[0], &sm[0], &offset[0]);
    if (!rc0) {
        while (1) {
            printf("Ooops, could not allocate inc_decoder_pulse_program\n");
        };
    }
    printf("Loaded inc_decoder_pulse_program at %u on pio %u\n", offset[0], PIO_NUM(pio[0]));

    inc_decoder_pulse_program_init(pio[0], sm[0], offset[0], PIO_INA_GPIO);
    pio_sm_exec(pio[0], sm[0], pio_encode_set(pio_x, 0));
    pio_sm_exec(pio[0], sm[0], pio_encode_set(pio_y, 0));

    pio_gpio_init(pio[0], PIO_INA_GPIO);
    pio_gpio_init(pio[0], PIO_INB_GPIO);

    bool rc1 = pio_claim_free_sm_and_add_program(&incremental_mpg_program, &pio[1], &sm[1], &offset[1]);
    if (!rc1) {
        while (1) {
            printf("Ooops, could not allocate inc_decoder_pulse_program\n");
        };
    }
    incremental_mpg_program_init(pio[1], sm[1], offset[1], PIO_STEP_GPIO);
    pio_sm_exec(pio[1], sm[1], pio_encode_set(pio_x, 0));
    pio_sm_exec(pio[1], sm[1], pio_encode_set(pio_y, 0));

    pio_gpio_init(pio[1], PIO_STEP_GPIO);
    pio_gpio_init(pio[1], PIO_DIR_GPIO);

    pio_sm_set_enabled(pio[1], sm[1], true);
    pio_sm_set_enabled(pio[0], sm[0], true);

    init_dma(pio[0], sm[0], pio[1], sm[1]);

    uint led = 0;

    while (true) {
        // debugging the fifos
        uint rxlevel1 = pio_sm_get_rx_fifo_level(pio[1], sm[1]);
        if (rxlevel1) {
            uint32_t received_val = pio_sm_get(pio[1], sm[1]);
            printf("gotcha1: %" PRIu32 "!\n", received_val);
            pico_set_led(led++ % 2);
        }
        // uint rxlevel0 = pio_sm_get_rx_fifo_level(pio[0], sm[0]);
        // if (rxlevel0) {
        //     uint32_t received_val = pio_sm_get(pio[0], sm[0]);
        //     printf("gotcha0: %" PRIu32 "!\n", received_val);
        //     pico_set_led(led++ % 2);
        // }
        else {
            printf("MPG running!\n");
            pico_set_led(led++ % 2);
            sleep_ms(LED_DELAY_MS);
        }
    }
}
