// Breath detection with the LSM6DS3 IMU (Arduino Nano 33 IoT). No WiFi, no phone.
//
// Pipeline:
//   fixed 100 Hz read -> low-pass (~1 Hz, kills jitter) -> subtract slow drift (~0.05 Hz)
//   -> calibration picks the best axis + measures your breath size
//   -> hysteresis state machine decides INHALE / EXHALE
//
// Wear it on the chest/belly and keep it in the same spot after calibration.
// Signal units are g (1 degree of tilt ~= 0.017 g).

#include <Arduino_LSM6DS3.h>

// ===================== Settings =====================
const bool  INHALE_RISES    = false;  // flip to false if INHALE/EXHALE come out reversed
const bool  PLOT_MODE       = false;   // true = print "signal,state" for Serial Plotter (no text)

const float SAMPLE_HZ       = 100.0;   // our sampling rate (IMU runs at 104 Hz)
const unsigned long SAMPLE_US = 10000UL; // 1,000,000 / SAMPLE_HZ

const float LOWPASS_HZ      = 1.0;     // block anything faster than this (jitter)
const float DRIFT_HZ        = 0.05;    // block anything slower than this (drift)

const unsigned long WARMUP_MS = 4000;  // let filters settle (LED blinks fast)
const unsigned long CALIB_MS  = 10000; // breathe normally and stay still (LED blinks slow)
const float MIN_SWING_G     = 0.004;   // calibration fails if breath swing is smaller (noise only)

const float HYST_FRAC       = 0.3;     // reversal must be 30% of your breath swing
const unsigned long MIN_PHASE_MS = 700; // an inhale or exhale can't be shorter than this

const float MOTION_DPS      = 25.0;    // gyro above this = body moved, ignore the data
const unsigned long MOTION_HOLD_MS = 1500;

// ===================== State =====================
enum Phase  { WARMUP, CALIBRATE, RUN };
enum Breath { UNKNOWN, INHALE, EXHALE };

Phase  phase = WARMUP;
Breath state = UNKNOWN;

float alphaLP, alphaDrift;
float lp[3], drift[3], band[3];     // per-axis filter values
bool  filtersInit = false;

unsigned long lastSampleUs = 0;
unsigned long phaseStartMs = 0;
unsigned long motionUntil  = 0;

// calibration
float sumSq[3];
unsigned long calibN = 0;
int   axis = 0;
float swing = 0;                    // typical valley-to-peak size of one breath (g)
float deadzone = 0;                 // reversal needed to change state (g)

// detector
float extremeVal = 0;               // highest point of current inhale / lowest of current exhale
float lo = 0, hi = 0;               // used only before the first state is known
unsigned long phaseSince = 0;
bool  needReset = true;
unsigned long breaths = 0;

// ===================== Helpers =====================
float alphaFor(float cutoffHz) {
  float rc = 1.0f / (2.0f * PI * cutoffHz);
  float dt = 1.0f / SAMPLE_HZ;
  return dt / (rc + dt);
}

void startWarmup(unsigned long now) {
  phase = WARMUP;
  phaseStartMs = now;
  if (!PLOT_MODE) Serial.println("Warming up... keep still");
}

void startCalibration(unsigned long now) {
  phase = CALIBRATE;
  phaseStartMs = now;
  calibN = 0;
  for (int i = 0; i < 3; i++) sumSq[i] = 0;
  if (!PLOT_MODE) Serial.println("Calibrating... breathe normally, keep still");
}

// Called when the PC sends 'R': start over from warmup + calibration.
void restartAll() {
  filtersInit = false;
  state = UNKNOWN;
  needReset = true;
  breaths = 0;
  motionUntil = 0;
  digitalWrite(LED_BUILTIN, LOW);
  startWarmup(millis());
}

void enterState(Breath s, float sig, unsigned long now) {
  Breath prev = state;
  state = s;
  extremeVal = sig;
  phaseSince = now;
  digitalWrite(LED_BUILTIN, s == INHALE ? HIGH : LOW);

  if (prev == EXHALE && s == INHALE) breaths++;   // exhale finished = one full breath

  if (!PLOT_MODE) {
    Serial.println(s == INHALE ? "INHALE" : "EXHALE");
    if (prev == EXHALE && s == INHALE) {
      Serial.print("breath #");
      Serial.println(breaths);
    }
  }
}

// Hysteresis: only flip after the signal reverses by more than `deadzone`
// from the highest (inhale) or lowest (exhale) point reached so far.
void detect(float sig, unsigned long now) {
  if (state == UNKNOWN) {
    if (sig < lo) lo = sig;
    if (sig > hi) hi = sig;
    if (sig - lo > deadzone)      enterState(INHALE, sig, now);
    else if (hi - sig > deadzone) enterState(EXHALE, sig, now);
    return;
  }

  bool longEnough = (now - phaseSince) >= MIN_PHASE_MS;

  if (state == INHALE) {
    if (sig > extremeVal) extremeVal = sig;
    else if (longEnough && (extremeVal - sig) > deadzone) enterState(EXHALE, sig, now);
  } else {  // EXHALE
    if (sig < extremeVal) extremeVal = sig;
    else if (longEnough && (sig - extremeVal) > deadzone) enterState(INHALE, sig, now);
  }
}

// ===================== Arduino =====================
void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 2000) {}   // wait briefly for a PC, but run fine without one

  pinMode(LED_BUILTIN, OUTPUT);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1) {                                  // fast blink = IMU error
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }

  alphaLP    = alphaFor(LOWPASS_HZ);
  alphaDrift = alphaFor(DRIFT_HZ);

  lastSampleUs = micros();
  startWarmup(millis());
}

void loop() {
  // ---- 0. PC command: 'R' = restart calibration ----
  while (Serial.available()) {
    if (Serial.read() == 'R') restartAll();
  }

  // ---- 1. fixed-rate timing: do nothing until 10 ms has passed ----
  unsigned long nowUs = micros();
  if (nowUs - lastSampleUs < SAMPLE_US) return;
  if (!IMU.accelerationAvailable() || !IMU.gyroscopeAvailable()) return;

  lastSampleUs += SAMPLE_US;                         // fixed schedule, no drift
  if (nowUs - lastSampleUs > 5 * SAMPLE_US) lastSampleUs = nowUs;  // fell far behind: resync

  float a[3], g[3];
  IMU.readAcceleration(a[0], a[1], a[2]);
  IMU.readGyroscope(g[0], g[1], g[2]);

  // ---- 2. band-pass each axis: low-pass minus slow drift ----
  if (!filtersInit) {
    for (int i = 0; i < 3; i++) { lp[i] = a[i]; drift[i] = a[i]; }
    filtersInit = true;
  }
  for (int i = 0; i < 3; i++) {
    lp[i]    += alphaLP    * (a[i]  - lp[i]);
    drift[i] += alphaDrift * (lp[i] - drift[i]);
    band[i]   = lp[i] - drift[i];
  }

  // ---- 3. motion gate (gyro): big rotation = body moved, not breathing ----
  unsigned long now = millis();
  float gyroMag = sqrtf(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);
  if (gyroMag > MOTION_DPS) motionUntil = now + MOTION_HOLD_MS;
  bool moving = (long)(motionUntil - now) > 0;

  // ---- 4. phases ----
  if (phase == WARMUP) {
    digitalWrite(LED_BUILTIN, (now / 150) % 2);
    if (moving) { phaseStartMs = now; return; }      // restart warmup if moved
    if (now - phaseStartMs >= WARMUP_MS) startCalibration(now);
    return;
  }

  if (phase == CALIBRATE) {
    digitalWrite(LED_BUILTIN, (now / 500) % 2);
    if (moving) { startCalibration(now); return; }   // restart if moved

    for (int i = 0; i < 3; i++) sumSq[i] += band[i] * band[i];
    calibN++;

    if (now - phaseStartMs >= CALIB_MS) {
      // best axis = the one that moved the most
      axis = 0;
      for (int i = 1; i < 3; i++) if (sumSq[i] > sumSq[axis]) axis = i;

      // For a smooth breath wave, valley-to-peak ~= 2.83 x RMS
      float rms = sqrtf(sumSq[axis] / calibN);
      swing = 2.828f * rms;

      if (swing < MIN_SWING_G) {
        if (!PLOT_MODE) {
          Serial.print("Breath too small to detect (swing ");
          Serial.print(swing, 4);
          Serial.println(" g). Check placement, retrying.");
        }
        startCalibration(now);
        return;
      }

      deadzone = HYST_FRAC * swing;
      needReset = true;
      phase = RUN;
      digitalWrite(LED_BUILTIN, LOW);

      if (!PLOT_MODE) {
        Serial.print("Calibration done. Axis: ");
        Serial.print("xyz"[axis]);
        Serial.print("  swing: ");
        Serial.print(swing, 4);
        Serial.print(" g  deadzone: ");
        Serial.print(deadzone, 4);
        Serial.println(" g");
        Serial.println("Running. LED on = inhale.");
      }
    }
    return;
  }

  // ---- RUN ----
  if (moving) { needReset = true; return; }          // ignore data while moving

  float sig = band[axis] * (INHALE_RISES ? 1.0f : -1.0f);

  if (needReset) {                                    // after motion, start fresh
    state = UNKNOWN;
    lo = hi = sig;
    needReset = false;
    digitalWrite(LED_BUILTIN, LOW);
  }

  detect(sig, now);

  if (PLOT_MODE) {
    static uint8_t n = 0;
    if (++n % 5 == 0) {                               // ~20 lines/sec
      Serial.print(sig, 4);
      Serial.print(",");
      Serial.println(state == INHALE ? swing / 2 : (state == EXHALE ? -swing / 2 : 0.0f), 4);
    }
  }
}
