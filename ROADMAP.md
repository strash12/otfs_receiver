# OTFS Receiver Roadmap

## Infrastructure

- [x] Repository
- [x] Architecture
- [x] CMake skeleton
- [x] Debug/Release presets
- [x] Common types
- [x] Aligned memory
- [ ] Timer
- [ ] Logger

## DSP

- [x] FFT backend
- [x] OTFS modulator: CP, RCP, ZP, RZP, NONE
- [x] OTFS demodulator: CP, RCP, ZP, RZP, NONE
- [x] Embedded pilot masks
- [x] Chirp synchronizer
- [ ] CFO estimator
- [ ] FRFT-SIC
- [ ] FID operator
- [ ] CG-LMMSE
- [ ] Protocol/FEC/CRC

## Runtime

- [ ] Offline receiver
- [ ] PlutoSDR
- [ ] USRP
- [ ] Realtime pipeline
- [ ] GUI

## DSP Infrastructure

- [x] Column-major grid view
- [x] OTFS workspace
- [x] DSP stage interface
- [x] Static pipeline
- [x] FFT plan cache

## MATLAB Regression

- [x] Binary complex-float reference format
- [x] C++ reference-data reader
- [x] OTFS MATLAB/C++ regression test
- [x] Pilot-mask exact comparison
- [ ] Generated MATLAB reference dataset committed

## Radio Front-End

- [x] Streaming DC remover
- [x] RMS/power meter
- [x] Optional RMS AGC
- [x] Front-end metrics
- [x] Allocation-free block processing
- [ ] FIR filtering
- [x] Chirp synchronizer
- [x] Coarse CFO
- [x] Fine CFO

## Synchronization Pipeline

- [x] Coarse chirp timing
- [x] Coarse CFO estimation
- [x] Coarse CFO correction
- [x] Refined chirp timing
- [x] Residual CFO estimation
- [x] Final CFO correction

## Frame Extraction and Resampling

- [x] Parameterized input/processing sample rates
- [x] Integer decimation factor validation
- [x] FIR anti-alias decimator
- [x] Factor 1 bypass
- [x] Factors 2 and 4
- [x] Parameterized frame offset
- [x] Pluto mapping: 4864 -> 2432
