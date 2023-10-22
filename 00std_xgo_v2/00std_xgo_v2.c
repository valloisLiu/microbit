#include <string.h>
#include "nrf52833.h"

/*
sources:
    https://wiki.elecfreaks.com/en/microbit/robot/xgo-robot-kit-v2/
    https://github.com/elecfreaks/pxt-xgo/blob/master/main.ts
    https://makecode.microbit.org/device/pins
    https://wiki.elecfreaks.com/en/pico/cm4-xgo-robot-kit/advanced-development/serial-protocol/
    
    UART
        tx==P2 onthe micro:bit silkscreen==P0.04 on the nRF (RING2 line on schematic)
        rx==P1 onthe micro:bit silkscreen==P0.03 on the nRF (RING1 line on schematic)
        baudrate: 115200
*/

uint8_t BUF_ACTION_SIT_DOWN[] = {
    0x55, // header 1
    0x00, // header 2
    0x09, // length
    0x00, // command_type
    0x3e, // address (0x3e==action command)
    0x0C, // data    (12==sit down)
    0xAC, // crc
    0x00, // end 1
    0xAA, // end 2
};

uint8_t uartvars_txBuf[9];
uint8_t uartvars_rxBuf[9];

int main(void) {
    
    // start hfclock
    NRF_CLOCK->EVENTS_HFCLKSTARTED     = 0;
    NRF_CLOCK->TASKS_HFCLKSTART        = 0x00000001;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    // configure P0.04 (UART TX) as output
    //  3           2            1           0
    // 1098 7654 3210 9876 5432 1098 7654 3210
    // .... .... .... .... .... .... .... ...A A: DIR:     1=Output
    // .... .... .... .... .... .... .... ..B. B: INPUT:   1=Disconnect
    // .... .... .... .... .... .... .... CC.. C: PULL:    0=Disabled
    // .... .... .... .... .... .DDD .... .... D: DRIVE:   0=S0S1
    // .... .... .... ..EE .... .... .... .... E: SENSE:   0=Disabled
    // xxxx xxxx xxxx xx00 xxxx xxxx xxxx 0011 
    //    0    0    0    0    0    0    0    3 0x00000003
    NRF_P0->PIN_CNF[4]                 = 0x00000003;

    // configure P0.03 (UART RX) as input
    //  3           2            1           0
    // 1098 7654 3210 9876 5432 1098 7654 3210
    // .... .... .... .... .... .... .... ...A A: DIR:     0=Input
    // .... .... .... .... .... .... .... ..B. B: INPUT:   0=Connect
    // .... .... .... .... .... .... .... CC.. C: PULL:    3=Pullup
    // .... .... .... .... .... .DDD .... .... D: DRIVE:   0=S0S1
    // .... .... .... ..EE .... .... .... .... E: SENSE:   0=Disabled
    // xxxx xxxx xxxx xx00 xxxx xxxx xxxx 1100 
    //    0    0    0    0    0    0    0    c 0x0000000c
    NRF_P0->PIN_CNF[3]                 = 0x0000000c;

    // configure
    NRF_UARTE0->TXD.PTR                = (uint32_t)uartvars_txBuf;
    NRF_UARTE0->TXD.MAXCNT             = sizeof(uartvars_txBuf);
    NRF_UARTE0->RXD.PTR                = (uint32_t)uartvars_rxBuf;
    NRF_UARTE0->RXD.MAXCNT             = sizeof(uartvars_rxBuf);
    NRF_UARTE0->PSEL.TXD               = 0x00000004; // 0x00000004==P0.04
    NRF_UARTE0->PSEL.RXD               = 0x00000003; // 0x00000003==P0.03
    NRF_UARTE0->CONFIG                 = 0x00000000; // 0x00000000==no flow control, no parity bits, 1 stop bit
    NRF_UARTE0->BAUDRATE               = 0x01D7E000; // 0x01D7E000==Baud115200
    
    //  3           2            1           0
    // 1098 7654 3210 9876 5432 1098 7654 3210
    // .... .... .... .... .... .... .... ...A A: CTS
    // .... .... .... .... .... .... .... ..B. B: NCTS
    // .... .... .... .... .... .... .... .C.. C: RXDRDY
    // .... .... .... .... .... .... ...D .... D: ENDRX
    // .... .... .... .... .... .... E... .... E: TXDRDY
    // .... .... .... .... .... ...F .... .... F: ENDTX
    // .... .... .... .... .... ..G. .... .... G: ERROR
    // .... .... .... ..H. .... .... .... .... H: RXTO
    // .... .... .... I... .... .... .... .... I: RXSTARTED
    // .... .... ...J .... .... .... .... .... J: TXSTARTED
    // .... .... .L.. .... .... .... .... .... L: TXSTOPPED
    // xxxx xxxx x0x0 0x0x xxxx xx01 0xx0 x000 
    //    0    0    0    0    0    1    0    0 0x00000100
    NRF_UARTE0->INTENSET               = 0x00000100;
    NRF_UARTE0->ENABLE                 = 0x00000008; // 0x00000008==enable
    
    // enable interrupts
    NVIC_SetPriority(UARTE0_UART0_IRQn, 1);
    NVIC_ClearPendingIRQ(UARTE0_UART0_IRQn);
    NVIC_EnableIRQ(UARTE0_UART0_IRQn);

    memcpy(uartvars_txBuf,BUF_ACTION_SIT_DOWN,sizeof(BUF_ACTION_SIT_DOWN));

    NRF_UARTE0->TASKS_STARTTX          = 0x00000001;

    while(1);
}

void UARTE0_UART0_IRQHandler(void) {

    if (NRF_UARTE0->EVENTS_ENDTX == 0x00000001) {
        // byte sent to computer

        // clear
        NRF_UARTE0->EVENTS_ENDTX = 0x00000000;
        
    }
}
