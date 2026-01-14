#include "lecture_puissance.h"

// ESP32 analogRead() => 0..4095 (12 bits)
static constexpr float ADC_MAX = 4095.0f;
static constexpr float ADC_REF_V = 3.3f;

LecturePuissance::LecturePuissance(uint8_t adcPin, float ratioAperV, float vrms)
: _adcPin(adcPin),
  _ratio(ratioAperV),
  _vrms(vrms),
  _samplePeriodUs(1000),
  _calibrated(false),
  _calibStartMs(0),
  _calibDurationMs(10000),
  _calibSum(0.0),
  _calibCount(0),
  _offsetLSB(0.0f),
  _rmsWindowMs(10000),
  _rmsStartMs(0),
  _sumSquare(0.0),
  _rmsCount(0),
  _lastIrmsA(0.0f),
  _lastPkW(0.0f)
{}

void LecturePuissance::begin(uint32_t samplePeriodUs)
{
  _samplePeriodUs = samplePeriodUs;
  pinMode(_adcPin, INPUT);
}

void LecturePuissance::startCalibration(uint32_t durationMs)
{
  _calibrated = false;
  _calibDurationMs = durationMs;
  _calibStartMs = millis();
  _calibSum = 0.0;
  _calibCount = 0;

  // Reset RMS
  _rmsStartMs = millis();
  _sumSquare = 0.0;
  _rmsCount = 0;
  _lastIrmsA = 0.0f;
  _lastPkW = 0.0f;
}

uint16_t LecturePuissance::readADC()
{
  return analogRead(_adcPin);
}

bool LecturePuissance::sample()
{
  const uint32_t nowMs = millis();
  const uint16_t adc = readADC();

  // 1) Calibration
  if (!_calibrated) {
    _calibSum += adc;
    _calibCount++;

    if ((nowMs - _calibStartMs) >= _calibDurationMs && _calibCount > 0) {
      _offsetLSB = (float)(_calibSum / (double)_calibCount);
      _calibrated = true;

      // démarre la fenêtre RMS
      _rmsStartMs = nowMs;
      _sumSquare = 0.0;
      _rmsCount = 0;
    }
    return false;
  }

  // 2) Accum RMS en LSB
  const float v_lsb = (float)adc - _offsetLSB;
  _sumSquare += (double)v_lsb * (double)v_lsb;
  _rmsCount++;

  // 3) Fin de fenêtre
  if ((nowMs - _rmsStartMs) >= _rmsWindowMs && _rmsCount > 0) {
    const float rms_lsb = sqrtf((float)(_sumSquare / (double)_rmsCount));
    const float rms_v = rms_lsb * (ADC_REF_V / ADC_MAX);
    _lastIrmsA = rms_v * _ratio;
    _lastPkW = (_lastIrmsA * _vrms) / 1000.0f;

    // reset
    _rmsStartMs = nowMs;
    _sumSquare = 0.0;
    _rmsCount = 0;

    return true;
  }

  return false;
}
