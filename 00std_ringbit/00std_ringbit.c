#include <string.h>
#include "nrf52833.h"

/*
sources:
    https://wiki.elecfreaks.com/en/microbit/robot/xgo-robot-kit-v2/
    https://github.com/elecfreaks/pxt-xgo/blob/master/main.ts
    https://makecode.microbit.org/device/pins
    
    UART
        tx==P2 onthe micro:bit silkscreen==P0.04 on the nRF (RING2 line on schematic)
        rx==P1 onthe micro:bit silkscreen==P0.03 on the nRF (RING1 line on schematic)
        baudrate: 115200
*/

uint8_t BUF_ACTION_SIT_DOWN[] = {0x55, 0x00, 0x09, 0x00, 0x3E, 0x0C, 0xAC, 0x00, 0xAA};

uint8_t uartvars_txBuf[9];
uint8_t uartvars_rxBuf[9];

int main(void) {
    
    // start hfclock
    NRF_CLOCK->EVENTS_HFCLKSTARTED     = 0;
    NRF_CLOCK->TASKS_HFCLKSTART        = 0x00000001;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    // configure
    NRF_UARTE0->RXD.PTR                = (uint32_t)uartvars_rxBuf;
    NRF_UARTE0->RXD.MAXCNT             = sizeof(uartvars_rxBuf);
    NRF_UARTE0->TXD.PTR                = (uint32_t)uartvars_txBuf;
    NRF_UARTE0->TXD.MAXCNT             = sizeof(uartvars_txBuf);
    NRF_UARTE0->PSEL.TXD               = 0x00000004; // 0x00000004==P0.04
    NRF_UARTE0->PSEL.RXD               = 0x00000003; // 0x00000003==P0.03
    NRF_UARTE0->CONFIG                 = 0x00000000; // 0x00000000==no flow control, no parity bits, 1 stop bit
    NRF_UARTE0->BAUDRATE               = 0x01D7E000; // 0x01D7E000==Baud115200
    //  3           2            1           0
    // 1098 7654 3210 9876 5432 1098 7654 3210
    // .... .... .... .... .... .... ..C. .... C: ENDRX_STARTRX
    // .... .... .... .... .... .... .D.. .... D: ENDRX_STOPRX
    // xxxx xxxx xxxx xxxx xxxx xxxx xx1x xxxx 
    //    0    0    0    0    0    0    2    0 0x00000020
    NRF_UARTE0->SHORTS                 = 0x00000020;
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
    // xxxx xxxx x0x0 0x0x xxxx xx01 0xx1 x000 
    //    0    0    0    0    0    1    1    0 0x00000110
    NRF_UARTE0->INTENSET               = 0x00000110;
    NRF_UARTE0->ENABLE                 = 0x00000008; // 0x00000008==enable
    
    // enable interrupts
    NVIC_SetPriority(UARTE0_UART0_IRQn, 1);
    NVIC_ClearPendingIRQ(UARTE0_UART0_IRQn);
    NVIC_EnableIRQ(UARTE0_UART0_IRQn);

    // start receiving
    NRF_UARTE0->EVENTS_RXSTARTED       = 0;
    NRF_UARTE0->TASKS_STARTRX          = 0x00000001; // 0x00000001==start RX state machine; read received byte from RXD register
    while (NRF_UARTE0->EVENTS_RXSTARTED == 0);

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

    if (NRF_UARTE0->EVENTS_ENDRX == 0x00000001) {
        // byte received from computer

        // clear
        NRF_UARTE0->EVENTS_ENDRX = 0x00000000;
    }
}
