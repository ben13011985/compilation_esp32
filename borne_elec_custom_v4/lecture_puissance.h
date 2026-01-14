#ifndef LECTURE_PUISSANCE_H
#define LECTURE_PUISSANCE_H

#include <Arduino.h>

/*
  LecturePuissance
  - Calibration offset (moyenne ADC) sur 10 s (non bloquant)
  - Calcul Irms sur fenêtre (par défaut 10 s)
  - Conversion en puissance active "approx" : P(kW) = Irms(A) * Vrms(V) / 1000
    (monophasé, cos(phi)=1 ; pour une estimation simple)
*/
class LecturePuissance {
public:
  LecturePuissance(uint8_t adcPin, float ratioAperV, float vrms = 230.0f);

  void begin(uint32_t samplePeriodUs = 1000);

  void startCalibration(uint32_t durationMs = 10000);

  // À appeler à chaque tick (idéalement 1 ms). Retourne true si une nouvelle valeur est prête.
  bool sample();

  bool isCalibrated() const { return _calibrated; }

  void setRmsWindow(uint32_t windowMs) { _rmsWindowMs = windowMs; }
  void setVrms(float vrms) { _vrms = vrms; }

  float getOffsetLSB() const { return _offsetLSB; }

  float getCurrentRMS_A() const { return _lastIrmsA; }
  float getPower_kW() const { return _lastPkW; }

private:
  uint8_t _adcPin;
  float _ratio;    // A / V RMS
  float _vrms;     // V
  uint32_t _samplePeriodUs;

  // calibration
  bool _calibrated;
  uint32_t _calibStartMs;
  uint32_t _calibDurationMs;
  double _calibSum;
  uint32_t _calibCount;
  float _offsetLSB;

  // rms window
  uint32_t _rmsWindowMs;
  uint32_t _rmsStartMs;
  double _sumSquare;
  uint32_t _rmsCount;

  float _lastIrmsA;
  float _lastPkW;

  uint16_t readADC();
};

#endif
