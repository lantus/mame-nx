/*************************************************************************

	Atari Video Pinball hardware

*************************************************************************/

/*----------- defined in machine/videopin.c -----------*/

INTERRUPT_GEN( videopin_interrupt );
WRITE_HANDLER( videopin_out1_w );
WRITE_HANDLER( videopin_out2_w );
WRITE_HANDLER( videopin_led_w );
WRITE_HANDLER( videopin_watchdog_w );
WRITE_HANDLER( videopin_ball_position_w );
WRITE_HANDLER( videopin_note_dvslrd_w );
READ_HANDLER( videopin_in0_r );
READ_HANDLER( videopin_in1_r );
READ_HANDLER( videopin_in2_r );


/*----------- defined in vidhrdw/videopin.c -----------*/

WRITE_HANDLER( videopin_videoram_w );
VIDEO_UPDATE( videopin );
