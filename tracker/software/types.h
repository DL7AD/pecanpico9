#ifndef __TYPES_H__
#define __TYPES_H__

typedef enum { // Modulation type
	MOD_NOT_SET,
	MOD_OOK,
	MOD_2FSK,
	MOD_2GFSK,
	MOD_AFSK
} mod_t;

// Protocol type
typedef enum {
	PROT_SSDV_2FSK,
	PROT_APRS_AFSK,
	PROT_APRS_2GFSK,
	PROT_UKHAS_2FSK,
	PROT_MORSE
} prot_t;

typedef enum {
	CONF_PARM,
	CONF_UNIT,
	CONF_EQNS,
	CONF_BITS
} telemetry_conf_t;

typedef enum {
	TEL_SATS,
	TEL_TTFF,
	TEL_VBAT,
	TEL_VSOL,
	TEL_PBAT,
	TEL_RBAT,
	TEL_PRESS,
	TEL_TEMP,
	TEL_HUM
} telemetry_t;

typedef struct {
	char callsign[16];			// APRS callsign
	uint8_t ssid;				// APRS SSID
	uint16_t symbol;			// APRS symbol
	char path[16];				// APRS path
	uint16_t preamble;			// Preamble in milliseconds
	telemetry_t tel[5];			// Telemetry types
	bool tel_enc;				// Transmit telemetry encoding information
	uint16_t tel_enc_cycle;		// Telemetry encoding cycle in seconds
	char tel_comment[32];		// Telemetry comment
} aprs_conf_t;

typedef enum {
	SLEEP_DISABLED,
	SLEEP_WHEN_VBAT_BELOW_THRES,
	SLEEP_WHEN_RBAT_BELOW_THRES,
	SLEEP_WHEN_VBAT_ABOVE_THRES,
	SLEEP_WHEN_RBAT_ABOVE_THRES,
	SLEEP_WHEN_DISCHARGING,
	SLEEP_WHEN_CHARGING
} sleep_type_t;

typedef struct {
	sleep_type_t type;
	uint16_t vbat_thres;
	uint16_t rbat_thres;
} sleep_conf_t;

typedef struct {
	uint8_t speed;		// OOK speed in wpm
} ook_conf_t;

typedef struct {
	uint8_t bits;
	uint8_t stopbits;
	uint16_t predelay;
	uint16_t baud;
	uint16_t shift;
} fsk_conf_t;

typedef struct {
	uint8_t dummy; // Not used yet
} afsk_conf_t;

typedef struct {
	uint32_t speed;
} gfsk_conf_t;

typedef struct { // Radio message type
	uint8_t 		msg[8191];		// Message (data)
	uint32_t		bin_len;		// Binary length
	uint32_t		freq;			// Frequency
	int8_t			power;			// Power in dBm
	mod_t			mod;			// Modulation

	ook_conf_t*		ook_conf;		// OOK config
	fsk_conf_t*		fsk_conf;		// 2FSK config
	afsk_conf_t*	afsk_conf;		// AFSK config
	gfsk_conf_t*	gfsk_conf;		// 2GFSK config
} radioMSG_t;

typedef enum {
	RES_QCIF,
	RES_QVGA,
	RES_VGA,
	RES_XGA,
	RES_UXGA,
	RES_MAX
} resolution_t;

typedef struct {
	char callsign[8];		// Callsign
	resolution_t res;		// Camera resolution
	uint8_t quality;		// JPEG quality
	uint8_t *ram_buffer;	// Camera Buffer
	uint32_t ram_size;		// Size of buffer
	uint32_t size_sampled;	// Actual image data size (do not set in config)
	bool redundantTx;		// Redundand packet transmission (APRS only)
} ssdv_conf_t;

typedef enum {
	FREQ_STATIC,			// Fixed frequency
	FREQ_APRS_REGION		// APRS region dependent (it changes it frequency automatically depending on which APRS frequency is used in this region)
} freq_type_t;

typedef struct {
	freq_type_t type;
	uint32_t hz;
} freq_conf_t;

typedef enum {
	TRIG_ONCE,				// Trigger once and never again (e.g. transmit specific position packet only at startup)
	TRIG_NEW_POINT,			// Triggered when new track point available
	TRIG_TIMEOUT,			// Triggered by timeout (e.g. trasmit position every 120sec)
	TRIG_CONTINUOUSLY		// Continue continuously (e.g. send new image once old image sent completely)
} trigger_type_t;

typedef struct {
	trigger_type_t type;	// Trigger type
	uint32_t timeout;		// Timeout in seconds
} trigger_conf_t;

typedef struct {
	char callsign[16];		// Callsign
	char format[150];		// Format
} ukhas_conf_t;

typedef struct {
	uint32_t log_size;
	uint32_t log_cycle;
	uint32_t log_num;
} log_conf_t;

typedef struct {
	char callsign[16];		// Callsign
	char format[50];		// Format
} morse_conf_t;

typedef struct {
	char				name[32];

	// Radio
	int8_t				power;
	freq_conf_t			frequency;
	prot_t				protocol;

	// Timing
	uint32_t			init_delay;
	uint32_t			packet_spacing;
	sleep_conf_t		sleep_conf;
	trigger_conf_t		trigger;

	// Modulation
	union {
		fsk_conf_t		fsk_conf;
		afsk_conf_t		afsk_conf;
		gfsk_conf_t		gfsk_conf;
		ook_conf_t		ook_conf;
	};

	// Protocol
	union {
		morse_conf_t	morse_conf;
		ukhas_conf_t	ukhas_conf;
		aprs_conf_t		aprs_conf;
	};
	ssdv_conf_t			ssdv_conf;

	// Watchdog
	systime_t			wdg_timeout;		// Time at which watchdog will reset the STM32, 0 inactive
} module_conf_t;

#endif

