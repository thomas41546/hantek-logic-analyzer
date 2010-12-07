/*
 * console.c
 *
 *  Created on: Dec 4, 2010
 *      Author: thomas
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>


#include "io.h"
#include "thread.h"
#include "local.h"

#define SAMPLERATE 5000000
#define BUFFER 10240
#define TRIG(tp)	(tp * 0xFFFE + 0xD7FE * (1 - tp))
static struct offset_ranges offset_ranges;
float nr_voltages[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5};
int voltage_ch[3] = { VOLTAGE_100mV, VOLTAGE_100mV};
static int coupling_ch[2] = { COUPLING_DC, COUPLING_DC };
float offset_ch[3] = { 0.5, 0.5, 0.5 }, offset_t = 0.66, position_t = 0.45;

static int ro_ch[2], ro_t;	// real offsets
static void set_offsets_c(){

	ro_ch[0] = 18747;
	ro_ch[1] = 19391;
	ro_t = 255;

	dso_set_offsets(ro_ch, ro_t);
}


int grabMyBuffer(){
	if(!dso_buffer_dirty)
		return 0;

	pthread_mutex_lock(&buffer_mutex);
	memcpy(my_buffer, dso_buffer, 2 * dso_buffer_size);
	dso_buffer_dirty = 0;
	pthread_mutex_unlock(&buffer_mutex);

	return 1;
}
void printCurrentBuffer(){

	for(int i = 0; i < 1; i++) {
		//printw("%+4.6f %+4.6f", (my_buffer[2*i + 1] - offset_ch[0] * 0xff) * nr_voltages[voltage_ch[0]] * 1/ 32.0, (my_buffer[2*i] - offset_ch[1] * 0xff) * nr_voltages[voltage_ch[1]] * 1 / 32.0);
		printw("%+4.9f %+4.9f", (my_buffer[2*i + 1] - offset_ch[0] * 0xff) * nr_voltages[voltage_ch[0]] /25 ,(my_buffer[2*i] - offset_ch[1] * 0xff) * nr_voltages[voltage_ch[1]] /25);
	}


}

int main(){


	dso_init();

	if(!dso_initialized){
		DMSG("Failed to initialize dso\n");
		return -1;
	}

	dso_adjust_buffer(BUFFER);

	dso_get_offsets(&offset_ranges);
	for(int i=0; i<2; i++) {
		DMSG("Channel %d\n", i + 1);
		for(int j=0; j<9; j++) {
			DMSG("%.2fV: %x - %x\n", nr_voltages[j], offset_ranges.channel[i][j][0], offset_ranges.channel[i][j][1]);
		}
		DMSG("\n");
	}
	DMSG("trigger: 0x%x - 0x%x\n", offset_ranges.trigger[0], offset_ranges.trigger[1]);

	DMSG("\n");

	dso_period_usec = (1000000.0 / SAMPLERATE * BUFFER);

	dso_thread_init();

	sleep(1);

	dso_configure(5, SELECT_CH1CH2, TRIGGER_CH1, SLOPE_PLUS, TRIG(position_t), BUFFER);
	dso_set_filter(0);
	dso_set_voltage(voltage_ch,coupling_ch,TRIGGER_CH1);

	for(int i = 0; i < 2; i++)
		ro_ch[i] = (offset_ranges.channel[i][voltage_ch[i]][1] - offset_ranges.channel[i][voltage_ch[i]][0]) * offset_ch[i] + offset_ranges.channel[i][voltage_ch[i]][0];
	ro_t = (offset_ranges.trigger[1] - offset_ranges.trigger[0]) * offset_t + offset_ranges.trigger[0];

	dso_thread_set_cb(&set_offsets_c);


	dso_thread_resume();

	initscr();

	for(int i=0; i < 10;){

		while(!grabMyBuffer()){
			usleep(10);
		}

		printCurrentBuffer();
		refresh();
		clear();

		usleep(1000*500);

	}
	endwin();			/* End curses mode		  */
	




	DMSG("terminating\n");
	dso_thread_terminate();
	DMSG("done\n");
	sleep(5);
	
	
	
	

}
