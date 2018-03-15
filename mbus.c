#include <lpc11xx.h>
#include "timer.h"
#include "mbus.h"
#include "leds.h"
#include "fifo.h"

extern fifo_t fifo_out;

/*
	Sniffer mode = 1

	����� ��������, � ������ ������ ������ �������� �� ���� ���� � ����.
	��������� ��� ������� ��������� � ����� �����.

	X4 - SSEL1 - P2.0
	X5 - SCK1  - P2.1
	X6 - MISO1 - P2.2
	X7 - MOSI1 - P2.3

	�������� ��������� ������� ����� �� ����:
	1. FF - �������� ����
	2. S - �������� BUSY (=0).
	3. U - ���������� BUSY (=1).
 */
 
#define MBUS_SOFT_USE 1

#if (MBUS_SOFT_USE == 1)
#define PIN_MBUSY_OUT    
#define PIN_MBUSY_IN     (LPC_GPIO2->DIR  &= ~(1UL<<0))
#define PIN_MBUSY_SET0   {PIN_MBUSY_OUT; LPC_GPIO2->DATA &= ~(1UL<<0); }
#define PIN_MBUSY_SET1   {PIN_MBUSY_IN;}
#else
#define PIN_MBUSY_OUT    (LPC_GPIO2->DIR  |=  (1UL<<0))
#define PIN_MBUSY_IN     (LPC_GPIO2->DIR  &= ~(1UL<<0))
#define PIN_MBUSY_SET0   (LPC_GPIO2->DATA &= ~(1UL<<0))
#define PIN_MBUSY_SET1   (LPC_GPIO2->DATA |=  (1UL<<0))
#endif
#define PIN_MBUSY_GET    (LPC_GPIO2->DATA &   (1UL<<0))

#if (MBUS_SOFT_USE == 1)
#define PIN_MCLOCK_OUT   
#define PIN_MCLOCK_IN    (LPC_GPIO2->DIR  &= ~(1UL<<1))
#define PIN_MCLOCK_SET0  {PIN_MCLOCK_OUT; LPC_GPIO2->DATA &= ~(1UL<<1);}
#define PIN_MCLOCK_SET1  {PIN_MCLOCK_IN;}
#else
#define PIN_MCLOCK_OUT   (LPC_GPIO2->DIR  |=  (1UL<<1))
#define PIN_MCLOCK_IN    (LPC_GPIO2->DIR  &= ~(1UL<<1))
#define PIN_MCLOCK_SET0  (LPC_GPIO2->DATA &= ~(1UL<<1))
#define PIN_MCLOCK_SET1  (LPC_GPIO2->DATA |=  (1UL<<1))
#endif
#define PIN_MCLOCK_GET   (LPC_GPIO2->DATA &   (1UL<<1))

#if (MBUS_SOFT_USE == 1)
#define PIN_MDATA_OUT    
#define PIN_MDATA_IN     (LPC_GPIO2->DIR  &= ~(1UL<<2))
#define PIN_MDATA_SET0   {PIN_MDATA_OUT; LPC_GPIO2->DATA &= ~(1UL<<2);}
#define PIN_MDATA_SET1   {PIN_MDATA_IN;}
#else
#define PIN_MDATA_OUT    (LPC_GPIO2->DIR  |=  (1UL<<2))
#define PIN_MDATA_IN     (LPC_GPIO2->DIR  &= ~(1UL<<2))
#define PIN_MDATA_SET0   (LPC_GPIO2->DATA &= ~(1UL<<2))
#define PIN_MDATA_SET1   (LPC_GPIO2->DATA |=  (1UL<<2))
#endif
#define PIN_MDATA_GET    (LPC_GPIO2->DATA &   (1UL<<2))

void mbus_init(int sniff) {
	/* Enable AHB clock to the GPIO domain. */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1UL<<6)|(1UL<<10)|(1UL<<16);

#ifdef __JTAG_DISABLED  
	LPC_IOCON->R_PIO1_1  &= ~0x07;
	LPC_IOCON->R_PIO1_1  |= 0x01;
#endif

	// Init M-BUSY, M_CLOCK, M-DATA in and pull-up.
	PIN_MBUSY_IN;
	PIN_MCLOCK_IN;
	PIN_MDATA_IN;

	// pio init:
	LPC_IOCON->PIO2_0 &= ~0x1F; /*  As  */
	LPC_IOCON->PIO2_0 |=  0x10; /* GPIO, pull up */
	LPC_IOCON->PIO2_1 &= ~0x1F; /*  As  */
	LPC_IOCON->PIO2_1 |=  0x10; /* GPIO, pull up */
	
	if (sniff) {
		// ----
		// Init M-CLOCK interrupt:
		// ----
		// Init ISENSE:
		// 0 = Interrupt on pin PIOn_x is configured as edge sensitive.
		// 1 = Interrupt on pin PIOn_x is configured as level sensitive. 
		LPC_GPIO2->IS &= ~((1UL<<0)|(1UL<<1));
		// Init IBE:
		// 0 = Interrupt on pin PIOn_x is controlled through register GPIOnIEV.
		// 1 = Both edges on pin PIOn_x trigger an interrupt.
		LPC_GPIO2->IBE |=  (1UL<<0);
		LPC_GPIO2->IBE &= ~(1UL<<1);
		// Init IEV:
		// 0 = Depending on setting in register GPIOnIS (see IS), falling edges or LOW level on pin PIOn_x trigger an interrupt.
		// 1 = Depending on setting in register GPIOnIS (see IS), rising edges or HIGH level on pin PIOn_x trigger an interrupt.
		LPC_GPIO2->IEV &= ~(1UL<<0);
		LPC_GPIO2->IEV |=  (1UL<<1);
		// Init IE:
		// 0 = Interrupt on pin PIOn_x is masked.
		// 1 = Interrupt on pin PIOn_x is not masked.
		LPC_GPIO2->IE |= (1UL<<0)|(1UL<<1);
		
		// Enable EINT2:
		NVIC_EnableIRQ(EINT2_IRQn);
	} else {
		NVIC_DisableIRQ(EINT2_IRQn);		
	}
}

static uint8_t bdata = 0;
static uint8_t boff = 0;

#if 0
//This is a function that sends a byte to the HU - (not using interrupts)
void mbus_sendb(uint8_t b){
	int n;
	NVIC_DisableIRQ(EINT2_IRQn); 
	PIN_MDATA_OUT;
  	for(n = 7; n >= 0; n--) {
    	while (PIN_MCLOCK_GET == 1);
		delay_mks(5);
    	if(b & (1<<n)) {
      		PIN_MDATA_SET1;
    	} else {
      		PIN_MDATA_SET0;
    	}  
    	while (PIN_MCLOCK_GET == 0);
		delay_mks(5);
  	} 
  	PIN_MDATA_IN;
  	NVIC_EnableIRQ(EINT2_IRQn);
}
#endif

void mbus_msend(uint8_t *data, int len) {
	int offset = 0;
	uint8_t b;
	while (PIN_MBUSY_GET == 0);
	NVIC_DisableIRQ(EINT2_IRQn); 
	delay_mks(2*1000);
	PIN_MDATA_OUT;
	PIN_MDATA_SET1;
	PIN_MCLOCK_OUT;
	PIN_MCLOCK_SET1;
	PIN_MBUSY_OUT;
	PIN_MBUSY_SET0;
	delay_mks(2*1000);
	while (offset < len) {
		int n;
		b = data[offset];
	  	for(n = 7; n >= 0; n--) {
	    	PIN_MCLOCK_SET0;
	    	if(b & (1<<n)) {
	      		PIN_MDATA_SET1;
	    	} else {
	      		PIN_MDATA_SET0;
	    	}  
			delay_mks(50);
	    	PIN_MCLOCK_SET1;
			delay_mks(70);
	  	} 
   		//PIN_MDATA_SET1;
		delay_mks(100);
		offset++;
	}
	delay_mks(3*1000);
  	PIN_MBUSY_SET1;
	delay_mks(100);
  	PIN_MDATA_IN;
  	PIN_MCLOCK_IN;
  	PIN_MBUSY_IN;
	delay_mks(100);
	LPC_GPIO2->IC = LPC_GPIO2->MIS;
  	NVIC_EnableIRQ(EINT2_IRQn);
}

void mbus_msend_slave(uint8_t *data, int len) {
	int offset = 0;
	uint8_t b;
	while (PIN_MBUSY_GET == 1);
	NVIC_DisableIRQ(EINT2_IRQn); 
	delay_mks(2*1000);
	PIN_MDATA_OUT;
	PIN_MDATA_SET1;
	PIN_MCLOCK_OUT;
	PIN_MCLOCK_SET1;
	//PIN_MBUSY_OUT;
	//PIN_MBUSY_SET0;
	delay_mks(1*1000);
	while (offset < len) {
		int n;
		b = data[offset];
	  	for(n = 7; n >= 0; n--) {
	    	PIN_MCLOCK_SET0;
	    	if(b & (1<<n)) {
	      		PIN_MDATA_SET1;
	    	} else {
	      		PIN_MDATA_SET0;
	    	}  
			delay_mks(50);
	    	PIN_MCLOCK_SET1;
			delay_mks(70);
	  	} 
   		PIN_MDATA_SET1;
		delay_mks(200);
		offset++;
	}
	//delay_mks(10*1000);
  	//PIN_MBUSY_SET1;
	while (PIN_MBUSY_GET == 0);
  	PIN_MDATA_IN;
  	PIN_MCLOCK_IN;
  	//PIN_MBUSY_IN;
	//delay_mks(100);
  	NVIC_EnableIRQ(EINT2_IRQn);
}

void PIOINT2_IRQHandler(void) {
	uint32_t status = LPC_GPIO2->MIS;
	if (status & (1UL<<0)) {
		if (PIN_MBUSY_GET) {
			led_green(0);
			//LPC_UART->THR  = 'U';
			fifo_put(&fifo_out, 'U');
			fifo_put(&fifo_out, '\r');
		} else {
			led_green(1);
			//LPC_UART->THR  = 'S';
			fifo_put(&fifo_out, 'S');
			// ��� ��������� BUSY-�������, �������� ������� ������ �����.
			boff = 0;
		}
	}
	if (status & (1UL<<1)) {
		if (boff == 0) {
			bdata = 0;
		}
		if (PIN_MDATA_GET) {
			bdata |= (0x80>>boff);
		}
		boff++;
		if (boff == 8) {
			boff = 0;

			if ((bdata&0xF0) >= 0xA0) {
				//LPC_UART->THR  = 'A'+((bdata&0xF0)>>4)-10;
				fifo_put(&fifo_out, 'A'+((bdata&0xF0)>>4)-10);
			} else {
				//LPC_UART->THR  = '0'+((bdata&0xF0)>>4);
				fifo_put(&fifo_out, '0'+((bdata&0xF0)>>4));
			}
			//while (!(LPC_UART->LSR & 0x20));
			if ((bdata&0x0F) >= 0x0A) {
				//LPC_UART->THR  = 'A'+(bdata&0x0F)-10;
				fifo_put(&fifo_out, 'A'+(bdata&0x0F)-10);
			} else {
				//LPC_UART->THR  = '0'+(bdata&0x0F);
				fifo_put(&fifo_out, '0'+(bdata&0x0F));
			}
		}
	}
	LPC_GPIO2->IC = status;
}

void mbus_init_DEV(void) {
	NVIC_DisableIRQ(EINT2_IRQn); 

	while (PIN_MBUSY_GET == 0);
	while (PIN_MBUSY_GET == 1);
	while (PIN_MBUSY_GET == 0); 
	delay_mks(10*1000);
  
	PIN_MBUSY_OUT;
	PIN_MBUSY_SET0;
	delay_mks(1200*1000);
	PIN_MBUSY_SET1;
	PIN_MBUSY_IN;

	NVIC_EnableIRQ(EINT2_IRQn); 
}

