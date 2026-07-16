# FRFT-SIC MATLAB Regression

В MATLAB:

```matlab
cd('/home/user/Desktop/otfs_receiver')
addpath('tests/matlab_reference')

generate_frft_sic_reference( ...
    'tests/matlab_reference/generated/frft_sic')
```

Затем:

```bash
cmake --build --preset debug
ctest --preset debug --output-on-failure
```
