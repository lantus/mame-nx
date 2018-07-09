/***************************************************************************

	Midway DCS Audio Board

****************************************************************************/

MACHINE_DRIVER_EXTERN( dcs_audio );
MACHINE_DRIVER_EXTERN( dcs_audio_uart );
MACHINE_DRIVER_EXTERN( dcs_audio_ram );

void dcs_init(void);
void dcs_ram_init(void);

void dcs_set_notify(void (*callback)(int));
int dcs_data_r(void);
int dcs_data2_r(void);
int dcs_control_r(void);
void dcs_data_w(int data);
void dcs_reset_w(int state);

