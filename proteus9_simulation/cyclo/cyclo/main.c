#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define ZCD PB0

#define T1 PB1
#define T2 PB2
#define T3 PB3
#define T4 PB4

#define N1 PD4
#define N2 PD5
#define N3 PD6
#define N4 PD7

#define PULSE_TICKS 74

volatile uint8_t input_positive = 0;
volatile uint8_t output_positive = 1;
volatile uint8_t scheduled_input, scheduled_output;
volatile uint8_t timer_state = 0;

volatile uint8_t freq_hz = 0;
volatile uint8_t phase_acc = 0;
volatile uint16_t firing_ticks = 2499;

volatile uint16_t stage_ms = 0;
volatile uint8_t stage = 0;

/* V/f = 5 V/Hz */
const uint16_t fire_table[41] =
{
	2499,

	2396, 2335, 2283, 2236, 2193,
	2152, 2114, 2076, 2040, 2005,   // 10 Hz = 50 V

	1971, 1937, 1903, 1871, 1838,
	1806, 1774, 1742, 1710, 1678,   // 20 Hz = 100 V

	1645, 1613, 1581, 1548, 1515,
	1482, 1448, 1414, 1379, 1344,   // 30 Hz = 150 V

	1307, 1270, 1232, 1192, 1151,
	1108, 1063, 1015, 965, 910      // 40 Hz = 200 V
};

void gates_off(void)
{
	PORTB &= ~((1<<T1)|(1<<T2)|(1<<T3)|(1<<T4));
	PORTD &= ~((1<<N1)|(1<<N2)|(1<<N3)|(1<<N4));
}

void set_point(uint8_t f)
{
	freq_hz = f;

	if (f == 0)
	{
		firing_ticks = 2499;
		phase_acc = 0;
	}
	else
	{
		firing_ticks = fire_table[f];
	}
}

void fire_pair(void)
{
	gates_off();

	if (scheduled_output)
	{
		if (scheduled_input)
		PORTB |= (1<<T1) | (1<<T4);
		else
		PORTB |= (1<<T2) | (1<<T3);
	}
	else
	{
		if (scheduled_input)
		PORTD |= (1<<N2) | (1<<N3);
		else
		PORTD |= (1<<N1) | (1<<N4);
	}
}

void timer1_start(uint16_t x)
{
	TCCR1B = (1<<WGM12);
	TCNT1 = 0;
	OCR1A = x;
	TIFR1 = (1<<OCF1A);

	TCCR1B = (1<<WGM12)|(1<<CS11)|(1<<CS10);
}

void timer1_stop(void)
{
	TCCR1B = (1<<WGM12);
	TCNT1 = 0;
}

/* 1 ms timer */
ISR(TIMER2_COMPA_vect)
{
	stage_ms++;

	/* 0 -> 10 Hz in 5 s */
	if (stage == 0 && stage_ms >= 500)
	{
		stage_ms = 0;
		set_point(freq_hz + 1);

		if (freq_hz >= 10)
		stage = 1;
	}

	/* Hold 10 Hz / 50 V for 10 s */
	else if (stage == 1 && stage_ms >= 2000)
	{
		stage_ms = 0;
		stage = 2;
	}

	/* 10 -> 20 Hz in 5 s */
	else if (stage == 2 && stage_ms >= 500)
	{
		stage_ms = 0;
		set_point(freq_hz + 1);

		if (freq_hz >= 20)
		stage = 3;
	}

	/* Hold 20 Hz / 100 V for 10 s */
	else if (stage == 3 && stage_ms >= 2000)
	{
		stage_ms = 0;
		stage = 4;
	}

	/* 20 -> 40 Hz in 5 s */
	else if (stage == 4 && stage_ms >= 250)
	{
		stage_ms = 0;
		set_point(freq_hz + 1);

		if (freq_hz >= 40)
		stage = 5;
	}

	/* Hold 40 Hz / 200 V for 10 s */
	else if (stage == 5 && stage_ms >= 2000)
	{
		stage_ms = 0;
		stage = 6;
	}

	/* 40 -> 0 Hz in 5 s */
	else if (stage == 6 && stage_ms >= 500)
	{
		stage_ms = 0;

		if (freq_hz >= 4)
		set_point(freq_hz - 4);
		else
		{
			set_point(0);
			stage = 7;
		}
	}
}

ISR(PCINT0_vect)
{
	if (!(PINB & (1<<ZCD)))
	return;

	timer1_stop();
	gates_off();

	if (freq_hz == 0)
	{
		input_positive ^= 1;
		return;
	}

	scheduled_input = input_positive;
	scheduled_output = output_positive;

	timer_state = 1;
	timer1_start(firing_ticks);

	input_positive ^= 1;

	phase_acc += freq_hz;

	if (phase_acc >= 50)
	{
		phase_acc -= 50;
		output_positive ^= 1;
	}
}

ISR(TIMER1_COMPA_vect)
{
	if (timer_state == 1)
	{
		fire_pair();
		timer_state = 2;
		timer1_start(PULSE_TICKS);
	}
	else
	{
		gates_off();
		timer1_stop();
		timer_state = 0;
	}
}

int main(void)
{
	DDRB &= ~(1<<ZCD);

	DDRB |= (1<<T1)|(1<<T2)|(1<<T3)|(1<<T4);
	DDRD |= (1<<N1)|(1<<N2)|(1<<N3)|(1<<N4);

	gates_off();

	/* Timer1 - SCR firing */
	TCCR1A = 0;
	TCCR1B = (1<<WGM12);
	TIMSK1 = (1<<OCIE1A);

	/* Timer2 - 1 ms timebase */
	TCCR2A = (1<<WGM21);
	OCR2A = 249;
	TCCR2B = (1<<CS22);
	TIMSK2 = (1<<OCIE2A);

	PCICR |= (1<<PCIE0);
	PCMSK0 |= (1<<PCINT0);

	sei();

	while (1)
	{
	}
}
