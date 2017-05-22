#ifndef __TYPES_H__
#define __TYPES_H__

typedef enum { // Modulation type
	MOD_OOK,
	MOD_2FSK,
	MOD_2GFSK,
	MOD_DOMINOEX16,
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
	CONFIG_PARM,
	CONFIG_UNIT,
	CONFIG_EQNS,
	CONFIG_BITS
} telemetry_config_t;

typedef enum {
	TEL_SATS,
	TEL_TTFF,
	TEL_VBAT,
	TEL_VSOL,
	TEL_PBAT,
	TEL_PSOL,
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
	bool tel_encoding;			// Transmit telemetry encoding information
	uint16_t tel_encoding_cycle;// Telemetry encoding cycle in seconds
	char tel_comment[32];		// Telemetry comment
} aprs_config_t;

typedef enum {
	SLEEP_DISABLED,
	SLEEP_WHEN_VBAT_BELOW_THRES,
	SLEEP_WHEN_VSOL_BELOW_THRES,
	SLEEP_WHEN_VBAT_ABOVE_THRES,
	SLEEP_WHEN_VSOL_ABOVE_THRES,
	SLEEP_WHEN_DISCHARGING,
	SLEEP_WHEN_CHARGING,
	SLEEP_WHEN_INSIDE_ITU1,
	SLEEP_WHEN_INSIDE_ITU2,
	SLEEP_WHEN_INSIDE_ITU3
} sleep_type_t;

typedef struct {
	sleep_type_t type;
	uint16_t vbat_thres;
	uint16_t vsol_thres;
} sleep_config_t;

typedef struct {
	uint8_t speed;		// OOK speed in wpm
} ook_config_t;

typedef struct {
	uint8_t bits;
	uint8_t stopbits;
	uint16_t predelay;
	uint16_t baud;
	uint16_t shift;
} fsk_config_t;

typedef struct {
	uint8_t dummy; // Not used yet
} afsk_config_t;

typedef struct {
	uint8_t dummy; // Not used yet
} gfsk_config_t;

typedef struct { // Radio message type
	uint8_t 		msg[512];		// Message (data)
	uint32_t		bin_len;		// Binary length
	uint32_t		freq;			// Frequency
	int8_t			power;			// Power in dBm
	mod_t			mod;			// Modulation

	ook_config_t*	ook_config;		// OOK config
	fsk_config_t*	fsk_config;		// 2FSK config
	afsk_config_t*	afsk_config;	// AFSK config
	gfsk_config_t*	gfsk_config;	// 2GFSK config
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
	uint8_t *ram_buffer;	// Camera Buffer (do not set in config)
	size_t ram_size;		// Size of buffer (do not set in config)
	bool no_camera;			// Camera disabled
} ssdv_config_t;

typedef enum {
	FREQ_STATIC,
	FREQ_DYNAMIC
} freq_type_t;

typedef struct {
	freq_type_t type;
	uint32_t hz;
	void* method;
} freuquency_config_t;

typedef enum {
	TRIG_ONCE,				// Trigger once and never again (e.g. transmit specific position packet only at startup)
	TRIG_EVENT,				// Triggered by specific event (e.g. transmit when new track point available)
	TRIG_TIMEOUT,			// Triggered by timeout (e.g. trasmit position every 120sec)
	TRIG_CONTINOUSLY		// Continue continously (e.g. send new image once old image sent completely)
} trigger_type_t;

typedef enum {
	NO_EVENT,				// No event, triggered once and never again
	EVENT_NEW_POINT			// Triggered when new track point available
} event_t;

typedef struct {
	trigger_type_t type;	// Trigger type
	uint32_t timeout;		// Timeout in seconds
	event_t event;			// Trigger events
} trigger_config_t;

typedef struct {
	char callsign[16];		// Callsign
	char format[150];		// Format
} ukhas_config_t;

typedef struct {
	uint32_t log_size;
	uint32_t log_cycle;
	uint32_t log_num;
} log_config_t;

typedef struct {
	char callsign[16];		// Callsign
	char format[50];		// Format
} morse_config_t;

typedef struct {
	char				name[32];

	// Radio
	int8_t				power;
	freuquency_config_t	frequency;
	prot_t				protocol;

	// Timing
	uint32_t			init_delay;
	uint32_t			packet_spacing;
	sleep_config_t		sleep_config;
	trigger_config_t	trigger;

	// Modulation
	union {
		fsk_config_t	fsk_config;
		afsk_config_t	afsk_config;
		gfsk_config_t	gfsk_config;
		ook_config_t	ook_config;
	};

	// Protocol
	union {
		morse_config_t	morse_config;
		ukhas_config_t	ukhas_config;
		aprs_config_t	aprs_config;
	};
	log_config_t		log_config;
	ssdv_config_t		ssdv_config;

	// Satellite
	char				sat_tle1[70];
	char				sat_tle2[70];

	// Watchdog
	systime_t			last_update;
	bool				active;
} module_conf_t;

typedef struct {
	uint32_t time;	// Time of occurance in seconds
	uint16_t error;
} error_t;

#endif

