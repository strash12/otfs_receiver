# OTFS Receiver

Модульный исследовательский OTFS-приёмник на C++20.

## Первая сборка

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build

cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

## Архитектура

- `otfs_core` — DSP-ядро;
- `otfs_io` — IQ-файлы и эталонные данные;
- `otfs_radio` — PlutoSDR и USRP;
- `otfs_runtime` — realtime pipeline;
- `apps` — конечные приложения.

См. `docs/ARCHITECTURE.md`.
