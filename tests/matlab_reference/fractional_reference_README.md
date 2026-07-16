# Fractional Reference MATLAB Regression

Из корня репозитория в MATLAB:

```matlab
cd('/home/user/Desktop/otfs_receiver')
addpath('tests/matlab_reference')

generate_fractional_reference_set( ...
    'tests/matlab_reference/generated/fractional_reference')
```

Затем:

```bash
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Отдельный запуск:

```bash
./build/debug/tests/fractional_reference_regression_test \
    tests/matlab_reference/generated/fractional_reference
```
