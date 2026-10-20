/**
 * @file clawgrid_config.h
 * @brief Hardware Pin Maps, Network Configuration, and Node IDs
 */

#ifndef CLAWGRID_CONFIG_H
#define CLAWGRID_CONFIG_H

/* Node Identifiers */
#define NODE_ID_VOICE_FASTPATH     0x0001  /* Node A */
#define NODE_ID_INTELLIGENCE_STATE 0x0002  /* Node B */
#define NODE_ID_BROADCAST          0xFFFF

/* Wireless (ESP-NOW) Channel */
#define CLAWGRID_WIFI_CHANNEL      1

/* Node A (Voice & Fast-path) Pin Mapping (ESP32-S3-N16R8) */
#define NODE_A_PIR_PIN             6       /* 3-pin PIR sensor input */
#define NODE_A_RELAY_LIGHT_PIN     4       /* Low-voltage relay channel 1 */
#define NODE_A_RELAY_FAN_PIN       5       /* Low-voltage relay channel 2 */
#define NODE_A_RELAY_ACTIVE_LOW    true    /* Safe default: active-low relay board */
#ifndef CLAWGRID_PHYSICAL_OUTPUTS
#define CLAWGRID_PHYSICAL_OUTPUTS  0 /* Simulation builds never drive relay pins. */
#endif
#define NODE_A_STATUS_LED_PIN      48      /* Onboard WS2812 or RGB LED */

/* I2S Microphone Pin Mapping (INMP441 / PDM) */
#define I2S_MIC_SCK_PIN            12      /* Serial Clock (BCLK) */
#define I2S_MIC_WS_PIN             11      /* Word Select (LRCLK) */
#define I2S_MIC_SD_PIN             10      /* Serial Data (DOUT) */

/* I2S Speaker / DAC Pin Mapping (MAX98357A / Tone) */
#define I2S_SPK_BCLK_PIN           16
#define I2S_SPK_LRC_PIN            17
#define I2S_SPK_DIN_PIN            18

/* Safety Timing Parameters */
#define CLAWGRID_MIN_DWELL_MS      3000    /* 3s anti-cycle dwell time */
#define CLAWGRID_VACANCY_GRACE_MS  120000  /* 2 minutes grace period */
#define CLAWGRID_HEARTBEAT_MS      1000    /* 1 Hz heartbeat */

#endif /* CLAWGRID_CONFIG_H */
