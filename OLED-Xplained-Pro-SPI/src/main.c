#include <asf.h>
#include "tc.h"
#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

#define LED_LEFT			PIOA
#define LED_LEFT_ID			ID_PIOA
#define LED_LEFT_IDX		0
#define LED_LEFT_MASK		(1 << LED_LEFT_IDX)

#define LED_CENTER			PIOC
#define LED_CENTER_ID		ID_PIOC
#define LED_CENTER_IDX		30
#define LED_CENTER_MASK		(1 << LED_CENTER_IDX)

#define LED_RIGHT			PIOB
#define LED_RIGHT_ID		ID_PIOB
#define LED_RIGHT_IDX		2
#define LED_RIGHT_MASK		(1 << LED_RIGHT_IDX)

#define BUTTON_LEFT			PIOD
#define BUTTON_LEFT_ID		ID_PIOD
#define BUTTON_LEFT_IDX		28
#define BUTTON_LEFT_MASK	(1 << BUTTON_LEFT_IDX)

#define BUTTON_CENTER		PIOC
#define BUTTON_CENTER_ID	ID_PIOC
#define BUTTON_CENTER_IDX	31
#define BUTTON_CENTER_MASK	(1 << BUTTON_CENTER_IDX)

#define BUTTON_RIGHT		PIOA
#define BUTTON_RIGHT_ID		ID_PIOA
#define BUTTON_RIGHT_IDX	19
#define BUTTON_RIGHT_MASK	(1 << BUTTON_RIGHT_IDX)

/** Flags */
volatile char flag_left = 0;
volatile char flag_center = 0;
volatile char flag_right = 0;
volatile char flag_tc_left = 0;
volatile char flag_tc_center = 0;
volatile char flag_tc_right = 0;
volatile char flag_rtt = 0;


void toggle_led(Pio *p_pio, const uint32_t ul_mask);
void toggle_led(Pio *p_pio, const uint32_t ul_mask) {
	if (pio_get_output_data_status(p_pio, ul_mask)) {
		pio_clear(p_pio, ul_mask);
	}
	else {
		pio_set(p_pio, ul_mask);
	}
}


/** TC Init */
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq) {
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	uint32_t channel = 1;

	/* Configura o PMC */
	/* O TimerCounter é meio confuso
	o uC possui 3 TCs, cada TC possui 3 canais
	TC0 : ID_TC0, ID_TC1, ID_TC2
	TC1 : ID_TC3, ID_TC4, ID_TC5
	TC2 : ID_TC6, ID_TC7, ID_TC8
	*/
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  4Mhz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura e ativa interrupçcão no TC canal 0 */
	/* Interrupção no C */
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);

	/* Inicializa o canal 0 do TC */
	tc_start(TC, TC_CHANNEL);
}

void TC0_Handler(void);
void TC0_Handler(void) {
	volatile uint32_t ul_dummy;
	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 0);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */
	flag_tc_left = 1;
};

void TC1_Handler(void);
void TC1_Handler(void) {
	volatile uint32_t ul_dummy;
	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 1);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */
	flag_tc_center = 1;
};

void TC2_Handler(void);
void TC2_Handler(void) {
	volatile uint32_t ul_dummy;
	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 2);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */
	flag_tc_right = 1;
};

void RTT_Handler(void) {
	uint32_t ul_status = rtt_get_status(RTT);
	
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC ) {
		
	}
	
	if ((ul_status && RTT_SR_ALMS) == RTT_SR_ALMS){
		// é como se os 3 botões fossem pressionados ao mesmo tempo
		flag_left = !flag_left;
		flag_center = !flag_center;
		flag_right = !flag_right;
		flag_rtt = 1;
	}
}

static float get_time_rtt() {
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses) {
	uint32_t ul_previous_time;
	
	rtt_sel_source(RTT, 0);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	//	while(ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses + ul_previous_time);
	
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN | RTT_MR_RTTINCIEN);
}

/** Callback Functions */
void left_callback(void);
void left_callback(void) {
	flag_left = !flag_left;
}

void center_callback(void);
void center_callback(void) {
	flag_center = !flag_center;
}

void right_callback(void);
void right_callback(void) {
	flag_right = !flag_right;
}
 

void init(void);
void init() {
	// initialize LEDs
	pmc_enable_periph_clk(LED_LEFT_ID);
	pmc_enable_periph_clk(LED_CENTER_ID);
	pmc_enable_periph_clk(LED_RIGHT_ID);
	// config LEDs
	pio_configure(LED_LEFT, PIO_OUTPUT_0, LED_LEFT_MASK, PIO_DEFAULT);
	pio_configure(LED_CENTER, PIO_OUTPUT_0, LED_CENTER_MASK, PIO_DEFAULT);
	pio_configure(LED_RIGHT, PIO_OUTPUT_0, LED_RIGHT_MASK, PIO_DEFAULT);
	
	// initialize buttons
	pmc_enable_periph_clk(BUTTON_LEFT_ID);
	pmc_enable_periph_clk(BUTTON_CENTER_ID);
	pmc_enable_periph_clk(BUTTON_RIGHT_ID);
	// config buttons
	pio_configure(BUTTON_LEFT, PIO_INPUT, BUTTON_LEFT_MASK, PIO_PULLUP);
	pio_configure(BUTTON_CENTER, PIO_INPUT, BUTTON_CENTER_MASK, PIO_PULLUP);
	pio_configure(BUTTON_RIGHT, PIO_INPUT, BUTTON_RIGHT_MASK, PIO_PULLUP);
	
	// config interruption
	pio_handler_set(BUTTON_LEFT, BUTTON_LEFT_ID, BUTTON_LEFT_MASK,
		PIO_IT_RISE_EDGE, left_callback);
	pio_handler_set(BUTTON_CENTER, BUTTON_CENTER_ID, BUTTON_CENTER_MASK,
		PIO_IT_FALL_EDGE, center_callback);
	pio_handler_set(BUTTON_RIGHT, BUTTON_RIGHT_ID, BUTTON_RIGHT_MASK,
		PIO_IT_RISE_EDGE, right_callback);
	
	// activate interruption
	pio_enable_interrupt(BUTTON_LEFT, BUTTON_LEFT_MASK);
	pio_enable_interrupt(BUTTON_CENTER, BUTTON_CENTER_MASK);
	pio_enable_interrupt(BUTTON_RIGHT, BUTTON_RIGHT_MASK);
	
	// config NVIC (priorities) (max = 0)
	NVIC_EnableIRQ(BUTTON_LEFT_ID);
	NVIC_SetPriority(BUTTON_LEFT_ID, 4);
	NVIC_EnableIRQ(BUTTON_CENTER_ID);
	NVIC_SetPriority(BUTTON_CENTER_ID, 4);
	NVIC_EnableIRQ(BUTTON_RIGHT_ID);
	NVIC_SetPriority(BUTTON_RIGHT_ID, 4);
	
	// default leds state
	pio_clear(LED_LEFT, LED_LEFT_MASK);
	pio_set(LED_CENTER, LED_CENTER_MASK);
	pio_clear(LED_RIGHT, LED_RIGHT_MASK);
	
	// config TCs || Config timer TC0, channel 1 at 4Hz
	// Devemos multiplicar por 2 porque a função "toggle" (alternar) vai
	// ser deve ser chamada o dobro do número de vezes que queremos que o 
	// led acenda.
	TC_init(TC0, ID_TC0, 0, 2*5);
	TC_init(TC0, ID_TC1, 1, 2*10);
	TC_init(TC0, ID_TC2, 2, 2*1);
	
	// RTT Init
	
	flag_rtt = 1;
	
}



int main (void) {
	init();
	board_init();
	sysclk_init();
	delay_init();

	// Init OLED
	gfx_mono_ssd1306_init();
	
	gfx_mono_draw_string("5Hz 10Hz 1Hz", 0, 16, &sysfont);

	/* Insert application code here, after the board has been initialized. */
	while(1) {
		if (flag_rtt) {
			uint16_t pllPreScale = (int) ((((float) 32768)/1.0));
			uint32_t irqRTTvalue = 1;
			RTT_init(pllPreScale, irqRTTvalue);
			flag_rtt = 0;
		}
		if (flag_tc_left) {
			if (flag_left) {
				toggle_led(LED_LEFT, LED_LEFT_MASK);
			}
			flag_tc_left = 0;
		}
		if (flag_tc_center) {
			if (flag_center) {
				toggle_led(LED_CENTER, LED_CENTER_MASK);
			}
			flag_tc_center = 0;
		}
		if (flag_tc_right) {
			if (flag_right) {
				toggle_led(LED_RIGHT, LED_RIGHT_MASK);
			}
			flag_tc_right = 0;
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
	
	return 0;
}
