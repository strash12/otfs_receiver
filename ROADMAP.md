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
- [ ] Embedded pilot masks
- [ ] Chirp synchronizer
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
