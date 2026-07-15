# MATLAB Reference Data

Запуск в MATLAB из корня репозитория:

```matlab
addpath('tests/matlab_reference');
generate_otfs_reference( ...
    'tests/matlab_reference/generated');
```

После генерации:

```bash
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Файлы `generated/` являются детерминированными эталонами MATLAB.
