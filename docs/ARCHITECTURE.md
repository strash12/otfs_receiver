# OTFS Receiver Software Architecture v1.0

## 1. Назначение

Проект `otfs_receiver` предназначен для реализации высокопроизводительного OTFS-приёмника на C++20 с возможностью работы:

- с офлайн IQ-записями;
- с ADALM-Pluto;
- с USRP;
- в однопоточном и многопоточном режимах;
- с последующим добавлением GUI и режима реального времени.

Архитектура должна обеспечить точное соответствие проверенному MATLAB-приёмнику, возможность покомпонентного тестирования и дальнейшую оптимизацию без изменения публичных интерфейсов.

---

## 2. Основные принципы

1. DSP-ядро не зависит от SDR, GUI, файловой системы и сетевых протоколов.
2. В горячем цикле обработки кадра запрещены динамические выделения памяти.
3. Все FFT-планы и рабочие буферы создаются заранее.
4. Формат DD-сетки сохраняет MATLAB-совместимую индексацию:
   `index = delay + doppler * M`.
5. Основной тип DSP-данных — `std::complex<float>`.
6. Скалярные накопления, нормы и внутренние произведения выполняются в `double`.
7. Каждый алгоритм сначала получает unit-test, затем интеграцию.
8. Для каждого перенесённого MATLAB-модуля создаётся эталонный тест MATLAB ↔ C++.
9. Внешние зависимости скрываются за узкими интерфейсами.
10. Любая оптимизация должна подтверждаться benchmark и не менять численный результат сверх установленного допуска.

---

## 3. Структура репозитория

```text
otfs_receiver/
├── apps/
│   ├── otfs_rx_offline/
│   ├── otfs_rx_live/
│   └── otfs_rx_gui/
│
├── libs/
│   ├── otfs_core/
│   ├── otfs_io/
│   ├── otfs_radio/
│   └── otfs_runtime/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── regression/
│   └── matlab_reference/
│
├── benchmarks/
├── tools/
├── docs/
├── scripts/
├── cmake/
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

---

## 4. Библиотеки

### 4.1 `otfs_core`

Чистое DSP-ядро.

Подмодули:

```text
otfs_core/
├── common/
├── memory/
├── fft/
├── waveform/
├── synchronization/
├── cfo/
├── channel_estimation/
├── fid/
├── equalization/
├── coding/
├── protocol/
├── recovery/
└── metrics/
```

`otfs_core` не содержит:

- `libiio`;
- UHD;
- Qt;
- файловых потоков;
- сокетов;
- потоков ОС;
- логики GUI.

### 4.2 `otfs_io`

Отвечает за:

- чтение и запись IQ-файлов;
- форматы `cf32`, `f32_iq`, `sc16`;
- replay захваченного сигнала;
- экспорт промежуточных массивов;
- обмен эталонными данными с MATLAB.

### 4.3 `otfs_radio`

Адаптеры оборудования:

- PlutoSDR через `libiio`;
- USRP через UHD;
- общий интерфейс источника отсчётов;
- установка частоты, частоты дискретизации и усиления;
- получение временных меток и статуса переполнения.

### 4.4 `otfs_runtime`

Организация выполнения:

- SPSC-очереди;
- пул кадровых буферов;
- поток приёма;
- поток DSP;
- поток телеметрии;
- backpressure;
- статистика задержек.

---

## 5. Форматы данных

### 5.1 Основные типы

```cpp
using sample_t = std::complex<float>;
using accumulator_t = std::complex<double>;
using real_accumulator_t = double;
```

### 5.2 DD-сетка

DD-сетка хранится линейно в column-major формате:

```cpp
index = delay + doppler * M;
```

где:

- `delay ∈ [0, M-1]`;
- `doppler ∈ [0, N-1]`.

Это соответствует MATLAB-линеаризации матрицы `M × N`.

### 5.3 Временной кадр

Для ZP-OTFS:

```text
frame_samples = (M + pad_length) * N
```

Для текущей конфигурации:

```text
M = 64
N = 32
pad_length = 12
frame_samples = 2432
```

---

## 6. Управление памятью

### 6.1 Требования

- выравнивание не менее 64 байт;
- повторное использование буферов;
- отсутствие `new`, `delete`, `malloc`, `free` внутри обработки кадра;
- отсутствие изменения размера `std::vector` в горячем цикле;
- заранее известная ёмкость очередей;
- отдельные рабочие области на каждый DSP-модуль.

### 6.2 Базовый контейнер

Предусматривается контейнер:

```cpp
template<class T>
class aligned_buffer;
```

Свойства:

- RAII;
- move-only;
- 64-byte alignment;
- `std::span<T>` для доступа;
- фиксированный размер после создания.

### 6.3 Пул кадров

В realtime-режиме используется фиксированный пул:

```text
free buffers -> RX thread -> DSP thread -> telemetry/output -> free buffers
```

Копирование IQ-кадров между этапами минимизируется.

---

## 7. FFT backend

Публичный интерфейс не зависит от FFTW:

```cpp
class fft_plan {
public:
    virtual void forward(
        std::span<const sample_t> input,
        std::span<sample_t> output) noexcept = 0;

    virtual void inverse(
        std::span<const sample_t> input,
        std::span<sample_t> output) noexcept = 0;
};
```

Первая реализация:

```text
FFTW3f
```

Требования:

- планы создаются при инициализации;
- используется `FFTW_MEASURE` или сохранённые wisdom;
- нормировка IFFT задаётся явно;
- FFTW-специфичные типы не выходят наружу;
- возможна дальнейшая замена на MKL, cuFFT или FFTW threads.

---

## 8. DSP pipeline

Общий тракт:

```text
Sample Source
    ↓
DC Removal
    ↓
Chirp Synchronization
    ↓
Coarse CFO Estimation
    ↓
CFO Correction
    ↓
Synchronization Refinement
    ↓
Residual CFO Estimation
    ↓
Frame Extraction
    ↓
Decimation
    ↓
OTFS Demodulation
    ↓
Embedded Pilot Processing
    ↓
Channel Estimation
    ↓
FID Channel Operator
    ↓
CG-LMMSE Equalization
    ↓
QPSK Demapping
    ↓
Header / FEC / CRC
    ↓
Recovery Controller
    ↓
Metrics / Output
```

---

## 9. Интерфейсы модулей

### 9.1 Синхронизация

```cpp
struct sync_result {
    bool found;
    std::size_t chirp_start;
    float normalized_correlation;
    float peak_to_background_db;
};

class chirp_synchronizer {
public:
    sync_result process(
        std::span<const sample_t> capture) noexcept;
};
```

Алгоритм должен соответствовать MATLAB-функции:

- FFT-convolution matched filter;
- ограничение валидной области поиска;
- нормированная корреляция `rho`;
- median background power;
- критерии `min_peak_db` и `min_rho`;
- кэширование спектра chirp.

### 9.2 CFO

```cpp
struct cfo_result {
    bool valid;
    double frequency_hz;
    double confidence;
};

class repeated_block_cfo_estimator;
class cfo_corrector;
```

Коррекция должна выполняться фазовым рекуррентным генератором либо заранее подготовленным phasor-буфером.

### 9.3 OTFS transform

```cpp
class otfs_modulator;
class otfs_demodulator;
```

Поддерживаемый первый режим:

```text
ZP
M = 64
N = 32
pad = 12
```

Требуется точное соответствие:

```matlab
y = ifft(x.').' / M
```

и

```matlab
y = fft(Y.').' * M
```

### 9.4 Маски embedded pilot

```cpp
struct pilot_masks {
    bit_mask data;
    bit_mask guard;
    bit_mask observation;
    bit_mask search;
    bit_mask noise;
};
```

Маски создаются один раз при инициализации.

### 9.5 Channel estimator

Публичный интерфейс:

```cpp
struct channel_path {
    sample_t gain;
    double delay_samples;
    double doppler_bins;
};

struct channel_estimate {
    std::span<const channel_path> paths;
    double noise_variance;
    double quality;
};

class channel_estimator {
public:
    channel_estimate estimate(
        std::span<const sample_t> frame) noexcept;
};
```

Первая полная реализация:

```text
FRFT-SIC
```

### 9.6 FID

```cpp
class fid_operator {
public:
    void set_channel(
        std::span<const channel_path> paths);

    void apply(
        std::span<const sample_t> input,
        std::span<sample_t> output) noexcept;

    void apply_adjoint(
        std::span<const sample_t> input,
        std::span<sample_t> output) noexcept;
};
```

Требования:

- без явной матрицы канала;
- fractional delay в частотной области;
- Doppler во временной области;
- FFT входа вычисляется один раз на `apply`;
- обязательный adjoint-test.

### 9.7 CG-LMMSE

```cpp
struct cg_options {
    std::size_t max_iterations;
    double tolerance;
    double regularization;
};

struct cg_result {
    bool converged;
    bool breakdown;
    std::size_t iterations;
    double relative_residual;
};

class cg_lmmse_equalizer;
```

Решаемая система:

```text
(GᴴG + λI)s = Gᴴr
```

Проверки:

- положительная кривизна;
- конечность коэффициентов;
- контроль комплексной части `pᴴAp`;
- остановка по относительной невязке;
- отсутствие аллокаций.

### 9.8 Coding и protocol

Модули:

- QPSK hard demapper;
- Hamming(7,4);
- CRC8;
- CRC16;
- PRBS31;
- header decoder;
- frame parser.

### 9.9 Recovery controller

Режимы:

```text
TRACK
KEYR
FULL
```

Контроллер выбирает режим по:

- CRC;
- FEC corrections;
- качеству канала;
- истории последних кадров;
- таймеру keyframe;
- результатам tracker.

---

## 10. Realtime architecture

Планируемая схема:

```text
Thread 1: Radio RX
    |
    | lock-free SPSC
    v
Thread 2: Synchronization + Frame Extraction
    |
    | lock-free SPSC
    v
Thread 3: OTFS DSP + Decode
    |
    | lock-free SPSC
    v
Thread 4: Metrics / GUI / Storage
```

На первом этапе допускается объединение потоков 2 и 3.

### Приоритеты

- Radio RX не блокируется DSP;
- при переполнении кадры отбрасываются контролируемо;
- счётчики overruns обязательны;
- GUI никогда не работает в DSP-потоке;
- логирование асинхронное.

---

## 11. Ошибки и исключения

Исключения разрешены:

- при конфигурации;
- при создании FFT-планов;
- при открытии файлов;
- при инициализации SDR.

Исключения запрещены:

- в `process()`;
- в обработке каждого кадра;
- в callback оборудования.

Realtime-функции используют:

```cpp
noexcept
```

и возвращают статус.

---

## 12. Логирование и метрики

Базовые метрики:

- sync peak dB;
- sync rho;
- coarse CFO;
- residual CFO;
- frame timing;
- channel paths;
- noise variance;
- CG iterations;
- CG residual;
- CRC status;
- corrected FEC words;
- EVM;
- SNR;
- processing time per stage;
- dropped frames;
- queue depth.

Метрики передаются отдельно от IQ-данных.

---

## 13. Тестирование

### 13.1 Unit tests

Каждый модуль тестируется изолированно.

Обязательные тесты:

- индексация DD-сетки;
- FFT/IFFT round-trip;
- OTFS mod/demod round-trip;
- pilot masks;
- chirp detection;
- CFO estimation;
- FID adjoint identity;
- CG identity-channel;
- CG multipath-channel;
- Hamming single-bit correction;
- CRC known vectors;
- PRBS reproducibility.

### 13.2 MATLAB regression

Для каждого крупного модуля MATLAB сохраняет:

```text
input.bin
expected_output.bin
metadata.json
```

C++-тест загружает их и проверяет:

```text
relative L2 error
maximum absolute error
phase error
index agreement
```

### 13.3 Integration tests

- synthetic frame without channel;
- integer delay;
- fractional delay;
- single Doppler;
- multipath;
- AWGN;
- CFO;
- timing offset;
- recorded Pluto capture.

### 13.4 Sanitizers

Debug preset:

- AddressSanitizer;
- UndefinedBehaviorSanitizer.

Дополнительно:

- Valgrind;
- ThreadSanitizer для runtime;
- static analysis `clang-tidy`.

---

## 14. Производительность

### 14.1 Benchmark targets

Измеряем:

- FFT;
- OTFS demod;
- chirp synchronizer;
- FID apply;
- FID adjoint;
- spectral norm;
- CG iteration;
- full frame processing.

### 14.2 Требования

Первые цели:

```text
1 OTFS frame processing < frame duration
no dynamic allocations per frame
stable latency without long-tail spikes
```

### 14.3 Оптимизации

Порядок оптимизации:

1. устранение аллокаций;
2. переиспользование FFT;
3. уменьшение копирований;
4. cache-friendly layout;
5. SIMD;
6. многопоточность;
7. platform-specific tuning.

---

## 15. Конфигурация

Конфигурация разделяется на:

```cpp
waveform_config
sync_config
cfo_config
channel_estimator_config
equalizer_config
protocol_config
runtime_config
radio_config
```

Конфигурационные структуры immutable после запуска pipeline.

Формат внешней конфигурации на первом этапе:

```text
TOML
```

---

## 16. Git workflow

Главная ветка:

```text
main
```

Для каждого этапа:

```text
feature/fft-backend
feature/otfs-transform
feature/chirp-sync
feature/cfo
feature/frft-sic
feature/fid
feature/cg
feature/pipeline
```

Правило:

```text
один законченный модуль = один Pull Request
```

Коммит допускается только при:

- успешной сборке;
- прохождении тестов;
- отсутствии новых предупреждений;
- обновлении документации.

---

## 17. Этапы реализации

### Stage 0 — Infrastructure

- структура репозитория;
- CMake;
- presets;
- warnings;
- sanitizers;
- smoke-test.

### Stage 1 — Memory and FFT

- aligned buffer;
- FFTW backend;
- FFT round-trip test;
- benchmark.

### Stage 2 — OTFS waveform

- ZP modulator;
- ZP demodulator;
- MATLAB regression;
- mask generator.

### Stage 3 — IQ and synchronization

- IQ reader;
- chirp synchronizer;
- offline capture test.

### Stage 4 — CFO

- coarse CFO;
- correction;
- residual CFO;
- synthetic regression.

### Stage 5 — Channel estimation

- observation extraction;
- FRFT primitives;
- FRFT-SIC;
- multipath tests.

### Stage 6 — FID

- `G`;
- `Gᴴ`;
- adjoint test;
- spectral norm.

### Stage 7 — Equalization

- CG-LMMSE;
- warm start;
- staged iterations;
- convergence metrics.

### Stage 8 — Protocol

- QPSK;
- header;
- Hamming;
- CRC;
- PRBS.

### Stage 9 — Receiver pipeline

- TRACK;
- KEYR;
- FULL;
- end-to-end offline receiver.

### Stage 10 — Radio

- Pluto;
- USRP;
- capture source abstraction.

### Stage 11 — Realtime

- queues;
- buffer pool;
- worker threads;
- telemetry.

### Stage 12 — GUI

- parameters;
- constellation;
- DD map;
- channel paths;
- EVM/SNR/CFO;
- logging.

---

## 18. Definition of Done

Модуль считается завершённым, когда:

- есть публичный интерфейс;
- есть реализация;
- есть unit-test;
- есть MATLAB regression, если применимо;
- нет аллокаций в горячем цикле;
- нет compiler warnings;
- проходит Debug + ASan/UBSan;
- проходит Release;
- обновлена документация;
- измерена производительность.

---

## 19. Первое архитектурное решение

Первый технический PR после Stage 0:

```text
feature/memory-fft
```

Состав:

- `aligned_buffer<T>`;
- FFTW3f backend;
- RAII FFT plans;
- forward/inverse API;
- unit-tests;
- benchmark executable;
- optional FFTW wisdom cache.

Это станет фундаментом OTFS, chirp synchronization, FID и FRFT.
