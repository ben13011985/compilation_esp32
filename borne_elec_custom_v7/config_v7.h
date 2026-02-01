#ifndef CONFIG_V7_H
#define CONFIG_V7_H

/************************************************************
 * CONFIGURATION GÉNÉRALE – BORNE ELEC v7
 ************************************************************/

/* ================= ADC ================= */
#define CFG_ADC_PIN                34
#define CFG_ADC_RESOLUTION_BITS    12
#define CFG_ADC_ATTENUATION        ADC_11db

/* ================= ACQUISITION ================= */
#define CFG_ACQ_PERIOD_MS          2
#define CFG_WINDOW_MS              10000

/* ================= CALIBRATION ================= */
#define CFG_CALIBRATION_MS         3000
#define CFG_CALIBRATE_AT_BOOT      1

/* ================= DEBUG ================= */
#define CFG_SERIAL_DEBUG           1
#define CFG_SERIAL_PERIOD_MS       10000

#endif // CONFIG_V7_H
