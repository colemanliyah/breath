# breath_as_input

Using breath as an input for machines. An Arduino Nano 33 IoT (LSM6DS3 accelerometer + gyroscope) worn on the chest or belly detects inhales and exhales with no phone and no microphone.

The goal of this repo is an Arduino library for breath detection. Two earlier projects, **BAM** and **Breathership**, live here as example projects.

## Layout

| Path | What it is |
|---|---|
| `sketches/breath_detect/` | Current working breath detector: fixed 100 Hz sampling, band-pass filter (0.05-1 Hz), automatic axis pick, hysteresis inhale/exhale detection. Prints `INHALE` / `EXHALE` over serial. |
| `tools/breath_scroll/` | Browser page (Chrome/Edge, Web Serial) that scrolls an image feed with the length of your exhale. Also a way to see the signal and tune it. |
| `projects/BAM/` | Breath Activated Meditation: wearable that darkens a polarized lens with each breath. Code, images, write-up. |
| `projects/Breathership/` | Two-person installation: a balloon inflates when two people's breathing syncs. Code, images, write-up. |

## Status

- Working: detector sketch and scroll page.
- Next: package the detector as a library (`src/`, `examples/`) and port BAM and Breathership to use it.

## Notes

- Never commit WiFi credentials. Put them in a `secrets.h` (ignored by git).
- The detector sketch accepts `R` over serial to restart calibration. Set `INHALE_RISES` if inhale/exhale come out reversed.

## Authors

Liyah Coleman and Brian Bishop. MIT license.
