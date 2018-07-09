/***************************************************************************

	Emulation of various Midway ICs

***************************************************************************/


/* 1st generation Midway serial PIC */
void midway_serial_pic_init(int upper);
void midway_serial_pic_reset_w(int state);
UINT8 midway_serial_pic_status_r(void);
UINT8 midway_serial_pic_r(void);
void midway_serial_pic_w(UINT8 data);


/* 2nd generation Midway serial/NVRAM/RTC PIC */
void midway_serial_pic2_init(int upper);
void midway_serial_pic2_reset_w(int state);
UINT8 midway_serial_pic2_status_r(void);
UINT8 midway_serial_pic2_r(void);
void midway_serial_pic2_w(UINT8 data);


/* I/O ASIC connected to 2nd generation PIC */
void midway_io_asic_init(int upper);
READ32_HANDLER( midway_io_asic_r );
WRITE32_HANDLER( midway_io_asic_w );


/* IDE ASIC maps the IDE registers */
READ32_HANDLER( midway_ide_asic_r );
WRITE32_HANDLER( midway_ide_asic_w );
