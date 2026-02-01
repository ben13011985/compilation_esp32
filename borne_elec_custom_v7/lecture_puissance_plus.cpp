#include "lecture_puissance_plus.h"
#include <math.h>

// ESP32 analogRead() : 12 bits (0..4095), Vref ~3.3V
static constexpr float ADC_MAX   = 4095.0f;
static constexpr float ADC_REF_V = 3.3f;

LecturePuissancePlus::LecturePuissancePlus(uint8_t pin_adc, float ratio_A_per_V, float vrms_mains)
: _pin(pin_adc), _ratio_A_per_V(ratio_A_per_V), _vrms_mains(vrms_mains) {}

void LecturePuissancePlus::begin(uint32_t sample_period_us) {
  _sample_period_us = sample_period_us;
  _window_start_ms = millis();
  resetWindowAccumulators();
}

void LecturePuissancePlus::setWindowMs(uint32_t window_ms) { _window_ms = window_ms; }
void LecturePuissancePlus::setRatio_A_per_V(float ratio) { _ratio_A_per_V = ratio; }
void LecturePuissancePlus::setVrmsMains(float vrms) { _vrms_mains = vrms; }

void LecturePuissancePlus::startCalibration(uint32_t duration_ms) {
  _calibrating = true;
  _calibrated  = false;
  _calib_end_ms = millis() + duration_ms;
  _calib_sum = 0;
  _calib_count = 0;
  _offset_lsb = 0.0f;
}

bool LecturePuissancePlus::isCalibrated() const { return _calibrated; }

void LecturePuissancePlus::resetWindowAccumulators() {
  _min_lsb = 0xFFFF;
  _max_lsb = 0;
  _sum_raw = 0;
  _sum_sq_centered = 0.0;
  _sum_abs_centered = 0.0;
  _count = 0;
}

bool LecturePuissancePlus::sample() {
  const uint32_t nowMs = millis();
  const uint16_t adc = (uint16_t)analogRead(_pin);

  // ---- Calibration offset ----
  if (_calibrating) {
    _calib_sum += adc;
    _calib_count++;
    if ((int32_t)(nowMs - _calib_end_ms) >= 0) {
      _offset_lsb = (_calib_count > 0) ? (float)_calib_sum / (float)_calib_count : 0.0f;
      _calibrating = false;
      _calibrated  = true;

      // Re-démarre une fenêtre propre après calibration
      _window_start_ms = nowMs;
      resetWindowAccumulators();
    }
    return false;
  }

  // Offset connu (sinon 0)
  const float offset = _calibrated ? _offset_lsb : 0.0f;
  const float centered = (float)adc - offset;

  // ---- Accumulateurs de fenêtre ----
  if (adc < _min_lsb) _min_lsb = adc;
  if (adc > _max_lsb) _max_lsb = adc;

  _sum_raw += adc;
  _sum_sq_centered += (double)centered * (double)centered;
  _sum_abs_centered += fabs((double)centered);
  _count++;

  // ---- Fin de fenêtre ? ----
  if ((uint32_t)(nowMs - _window_start_ms) >= _window_ms) {
    // Snapshot min/max/mean/count AVANT reset
    _last_min_lsb = (_min_lsb == 0xFFFF) ? 0 : _min_lsb;
    _last_max_lsb = _max_lsb;
    _last_mean_lsb = (_count > 0) ? (float)_sum_raw / (float)_count : 0.0f;
    _last_count = _count;

    // Intégration demandée : somme des |adc - offset|
    _last_sum_abs_centered_lsb = _sum_abs_centered;
    const double dt_ms = (double)_sample_period_us / 1000.0;
    _last_sum_abs_centered_lsb_ms = _sum_abs_centered * dt_ms;

    // RMS
    const double mean_sq = (_count > 0) ? (_sum_sq_centered / (double)_count) : 0.0;
    const double rms_lsb = sqrt(mean_sq);
    const double vrms_adc = rms_lsb * (ADC_REF_V / ADC_MAX);

    _last_irms_a = (float)(vrms_adc * (double)_ratio_A_per_V);
    _last_power_w = _last_irms_a * _vrms_mains;

    // Reset fenêtre
    _window_start_ms = nowMs;
    resetWindowAccumulators();
    return true;
  }

  return false;
}

// ===== Getters =====
float  LecturePuissancePlus::getIrmsA() const { return _last_irms_a; }
float  LecturePuissancePlus::getPowerW() const { return _last_power_w; }
float  LecturePuissancePlus::getPowerkW() const { return _last_power_w / 1000.0f; }

float  LecturePuissancePlus::getOffsetLSB() const { return _offset_lsb; }

uint16_t LecturePuissancePlus::getMinLSB() const { return _last_min_lsb; }
uint16_t LecturePuissancePlus::getMaxLSB() const { return _last_max_lsb; }
float    LecturePuissancePlus::getMeanLSB() const { return _last_mean_lsb; }

double LecturePuissancePlus::getSumAbsCenteredLSB() const { return _last_sum_abs_centered_lsb; }
double LecturePuissancePlus::getSumAbsCenteredLSBms() const { return _last_sum_abs_centered_lsb_ms; }

uint32_t LecturePuissancePlus::getWindowMs() const { return _window_ms; }
uint32_t LecturePuissancePlus::getSampleCountInLastWindow() const { return _last_count; }
