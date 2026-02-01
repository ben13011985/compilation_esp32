#pragma once
#include <Arduino.h>

class LecturePuissancePlus {
public:
  LecturePuissancePlus(uint8_t pin_adc, float ratio_A_per_V, float vrms_mains);

  // sample_period_us: cadence à laquelle TU appelles sample()
  void begin(uint32_t sample_period_us);

  // window_ms: durée de la fenêtre (stats + RMS calculés à la fin de chaque fenêtre)
  void setWindowMs(uint32_t window_ms);

  // Conversion
  void setRatio_A_per_V(float ratio);
  void setVrmsMains(float vrms);

  // Calibration offset (moyenne ADC sur duration_ms)
  void startCalibration(uint32_t duration_ms);
  bool isCalibrated() const;

  // À appeler à cadence fixe. Retourne true quand une fenêtre est terminée (nouveaux résultats)
  bool sample();

  // ===== Résultats (valides après la fin d'une fenêtre) =====
  float  getIrmsA() const;
  float  getPowerW() const;
  float  getPowerkW() const;

  // Offset de calibration (LSB)
  float  getOffsetLSB() const;

  // ===== Stats LSB (raw) sur la DERNIÈRE fenêtre terminée =====
  uint16_t getMinLSB() const;
  uint16_t getMaxLSB() const;
  float    getMeanLSB() const;

  // ===== Intégration demandée =====
  // Somme des écarts ABSOLUS par rapport à l'offset de calibration sur la fenêtre :
  //   sumAbs = Σ |adc - offset|
  // Deux formats :
  //   - en LSB (sans dimension de temps)
  //   - en LSB·ms (multiplié par dt_ms)
  double getSumAbsCenteredLSB() const;
  double getSumAbsCenteredLSBms() const;

  // Infos fenêtre
  uint32_t getWindowMs() const;
  uint32_t getSampleCountInLastWindow() const;

private:
  void resetWindowAccumulators();

private:
  uint8_t  _pin = 0;

  uint32_t _sample_period_us = 1000;
  uint32_t _window_ms = 1000;

  float _ratio_A_per_V = 1.0f;
  float _vrms_mains = 230.0f;

  // Calibration
  bool     _calibrating = false;
  bool     _calibrated  = false;
  uint32_t _calib_end_ms = 0;
  uint64_t _calib_sum = 0;
  uint32_t _calib_count = 0;
  float    _offset_lsb = 0.0f;

  // Fenêtre courante (accumulateurs)
  uint32_t _window_start_ms = 0;
  uint16_t _min_lsb = 0xFFFF;
  uint16_t _max_lsb = 0;
  uint64_t _sum_raw = 0;
  double   _sum_sq_centered = 0.0;
  double   _sum_abs_centered = 0.0;
  uint32_t _count = 0;

  // Derniers résultats (snapshot à la fin de fenêtre)
  uint16_t _last_min_lsb = 0;
  uint16_t _last_max_lsb = 0;
  float    _last_mean_lsb = 0.0f;
  uint32_t _last_count = 0;

  float  _last_irms_a = 0.0f;
  float  _last_power_w = 0.0f;
  double _last_sum_abs_centered_lsb = 0.0;
  double _last_sum_abs_centered_lsb_ms = 0.0;
};
