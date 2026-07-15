function generate_otfs_reference(outputDir)
%GENERATE_OTFS_REFERENCE Формирует эталонные данные MATLAB для C++ regression.
%
% Использование:
%   generate_otfs_reference('tests/matlab_reference/generated')
%
% Формат комплексных массивов:
%   float32 little-endian, чередование Re, Im:
%   Re(x1), Im(x1), Re(x2), Im(x2), ...
%
% Индексация DD-сетки:
%   MATLAB column-major, размер M x N.
%
% Формируются режимы:
%   CP, RCP, ZP, RZP, NONE.

    if nargin < 1 || isempty(outputDir)
        outputDir = fullfile(pwd, 'generated');
    end

    if ~exist(outputDir, 'dir')
        mkdir(outputDir);
    end

    cfg.M = 64;
    cfg.N = 32;
    cfg.padLength = 12;
    cfg.deltaF = 15e3;
    cfg.oversampling = 2;

    cfg.pilotDelay = 32;      % zero-based
    cfg.pilotDoppler = 16;    % zero-based
    cfg.pilotValue = single(15);
    cfg.Ltau = 14;
    cfg.KnuRegion = 2;

    rng(20260715, 'twister');

    [delayGrid, dopplerGrid] = ndgrid(0:cfg.M-1, 0:cfg.N-1);

    phase = 2*pi * ...
        (3*double(delayGrid) + 5*double(dopplerGrid)) / ...
        double(cfg.M * cfg.N);

    ddGrid = complex( ...
        single(cos(phase)), ...
        single(sin(phase)));

    % Добавляем несколько несимметричных детерминированных компонент,
    % чтобы тест ловил ошибки транспонирования и индексации.
    ddGrid(1, 1) = complex(single(0.75), single(-0.25));
    ddGrid(5, 8) = complex(single(-1.25), single(0.50));
    ddGrid(33, 17) = complex(cfg.pilotValue, single(0));

    writeComplexFloat32( ...
        fullfile(outputDir, 'dd_input.cf32'), ...
        ddGrid(:));

    modes = {'CP', 'RCP', 'ZP', 'RZP', 'NONE'};

    for modeIndex = 1:numel(modes)
        mode = modes{modeIndex};

        timeSignal = otfsModReference( ...
            ddGrid, ...
            cfg.padLength, ...
            mode);

        restoredGrid = otfsDemodReference( ...
            timeSignal, ...
            cfg.M, ...
            cfg.N, ...
            cfg.padLength, ...
            mode);

        writeComplexFloat32( ...
            fullfile(outputDir, ...
                sprintf('time_%s.cf32', lower(mode))), ...
            timeSignal(:));

        writeComplexFloat32( ...
            fullfile(outputDir, ...
                sprintf('dd_restored_%s.cf32', lower(mode))), ...
            restoredGrid(:));
    end

    masks = makePilotMasks(cfg);

    writeUint8( ...
        fullfile(outputDir, 'mask_data.u8'), ...
        masks.data(:));

    writeUint8( ...
        fullfile(outputDir, 'mask_guard.u8'), ...
        masks.guard(:));

    writeUint8( ...
        fullfile(outputDir, 'mask_observation.u8'), ...
        masks.observation(:));

    writeUint8( ...
        fullfile(outputDir, 'mask_search.u8'), ...
        masks.search(:));

    writeUint8( ...
        fullfile(outputDir, 'mask_noise.u8'), ...
        masks.noise(:));

    writeMetadata( ...
        fullfile(outputDir, 'metadata.txt'), ...
        cfg, ...
        masks);

    fprintf('MATLAB OTFS reference generated in:\n%s\n', outputDir);
    fprintf('DD cells: %d\n', cfg.M * cfg.N);
    fprintf('CP/ZP samples: %d\n', (cfg.M + cfg.padLength) * cfg.N);
    fprintf('RCP/RZP samples: %d\n', cfg.M * cfg.N + cfg.padLength);
    fprintf('NONE samples: %d\n', cfg.M * cfg.N);
    fprintf('Data/guard/observation/noise: %d/%d/%d/%d\n', ...
        nnz(masks.data), ...
        nnz(masks.guard), ...
        nnz(masks.observation), ...
        nnz(masks.noise));
end

function timeSignal = otfsModReference(ddGrid, padLength, mode)
    [M, N] = size(ddGrid);

    % Точное соответствие helperOTFSmod:
    % y = ifft(x.').' / M
    unprotected = ifft(ddGrid.', [], 1).' / M;
    unprotected = single(unprotected);

    switch upper(mode)
        case 'CP'
            output = complex(zeros(M + padLength, N, 'single'));
            output(1:padLength, :) = ...
                unprotected(M-padLength+1:M, :);
            output(padLength+1:padLength+M, :) = unprotected;
            timeSignal = output(:);

        case 'RCP'
            payload = unprotected(:);
            timeSignal = [ ...
                payload(end-padLength+1:end); ...
                payload];

        case 'ZP'
            output = complex(zeros(M + padLength, N, 'single'));
            output(1:M, :) = unprotected;
            timeSignal = output(:);

        case 'RZP'
            payload = unprotected(:);
            timeSignal = [ ...
                payload; ...
                complex(zeros(padLength, 1, 'single'))];

        case 'NONE'
            timeSignal = unprotected(:);

        otherwise
            error('Unsupported guard mode: %s', mode);
    end
end

function ddGrid = otfsDemodReference( ...
    timeSignal, M, N, padLength, mode)

    switch upper(mode)
        case 'CP'
            framed = reshape(timeSignal, M + padLength, N);
            unprotected = framed(padLength+1:padLength+M, :);

        case 'RCP'
            payload = timeSignal(padLength+1:end);
            unprotected = reshape(payload, M, N);

        case 'ZP'
            framed = reshape(timeSignal, M + padLength, N);
            unprotected = framed(1:M, :);

        case 'RZP'
            payload = timeSignal(1:M*N);
            unprotected = reshape(payload, M, N);

        case 'NONE'
            unprotected = reshape(timeSignal, M, N);

        otherwise
            error('Unsupported guard mode: %s', mode);
    end

    % Точное соответствие helperOTFSdemod:
    % y = fft(Y.').' * M
    ddGrid = fft(unprotected.', [], 1).' * M;
    ddGrid = single(ddGrid);
end

function masks = makePilotMasks(cfg)
    M = cfg.M;
    N = cfg.N;

    pilotDelayMatlab = cfg.pilotDelay + 1;
    pilotDopplerMatlab = cfg.pilotDoppler + 1;

    masks.data = true(M, N);
    masks.guard = false(M, N);
    masks.observation = false(M, N);
    masks.search = false(M, N);
    masks.noise = false(M, N);

    for delayOffset = -cfg.Ltau:cfg.Ltau
        delayIndex = pilotDelayMatlab + delayOffset;

        if delayIndex < 1 || delayIndex > M
            error('Pilot guard exceeds delay grid.');
        end

        for dopplerOffset = -2*cfg.KnuRegion:2*cfg.KnuRegion
            dopplerZeroBased = mod( ...
                cfg.pilotDoppler + dopplerOffset, ...
                N);

            dopplerIndex = dopplerZeroBased + 1;

            masks.guard(delayIndex, dopplerIndex) = true;
            masks.data(delayIndex, dopplerIndex) = false;
        end
    end

    for delayOffset = 0:cfg.Ltau
        delayIndex = pilotDelayMatlab + delayOffset;

        for dopplerOffset = -cfg.KnuRegion:cfg.KnuRegion
            dopplerZeroBased = mod( ...
                cfg.pilotDoppler + dopplerOffset, ...
                N);

            dopplerIndex = dopplerZeroBased + 1;

            masks.observation(delayIndex, dopplerIndex) = true;
            masks.search(delayIndex, dopplerIndex) = true;
        end
    end

    masks.noise = masks.guard & ~masks.observation;

    pilotIndex = sub2ind( ...
        [M, N], ...
        pilotDelayMatlab, ...
        pilotDopplerMatlab);

    assert(masks.guard(pilotIndex));
    assert(masks.observation(pilotIndex));
    assert(~masks.data(pilotIndex));
    assert(~masks.noise(pilotIndex));
end

function writeComplexFloat32(fileName, values)
    fileId = fopen(fileName, 'w', 'ieee-le');

    if fileId < 0
        error('Cannot open file: %s', fileName);
    end

    cleanup = onCleanup(@() fclose(fileId));

    values = single(values(:));

    interleaved = zeros(2*numel(values), 1, 'single');
    interleaved(1:2:end) = real(values);
    interleaved(2:2:end) = imag(values);

    count = fwrite(fileId, interleaved, 'single');

    if count ~= numel(interleaved)
        error('Incomplete write: %s', fileName);
    end
end

function writeUint8(fileName, values)
    fileId = fopen(fileName, 'w', 'ieee-le');

    if fileId < 0
        error('Cannot open file: %s', fileName);
    end

    cleanup = onCleanup(@() fclose(fileId));

    count = fwrite(fileId, uint8(values(:)), 'uint8');

    if count ~= numel(values)
        error('Incomplete write: %s', fileName);
    end
end

function writeMetadata(fileName, cfg, masks)
    fileId = fopen(fileName, 'w');

    if fileId < 0
        error('Cannot open metadata file: %s', fileName);
    end

    cleanup = onCleanup(@() fclose(fileId));

    fprintf(fileId, 'format_version=1\n');
    fprintf(fileId, 'complex_format=interleaved_float32_le\n');
    fprintf(fileId, 'layout=matlab_column_major\n');
    fprintf(fileId, 'M=%d\n', cfg.M);
    fprintf(fileId, 'N=%d\n', cfg.N);
    fprintf(fileId, 'pad_length=%d\n', cfg.padLength);
    fprintf(fileId, 'delta_f_hz=%.17g\n', cfg.deltaF);
    fprintf(fileId, 'oversampling=%d\n', cfg.oversampling);
    fprintf(fileId, 'pilot_delay_zero_based=%d\n', cfg.pilotDelay);
    fprintf(fileId, 'pilot_doppler_zero_based=%d\n', cfg.pilotDoppler);
    fprintf(fileId, 'pilot_value=%.9g\n', cfg.pilotValue);
    fprintf(fileId, 'maximum_delay_bins=%d\n', cfg.Ltau);
    fprintf(fileId, 'doppler_radius=%d\n', cfg.KnuRegion);
    fprintf(fileId, 'dd_cells=%d\n', cfg.M * cfg.N);
    fprintf(fileId, 'samples_cp=%d\n', ...
        (cfg.M + cfg.padLength) * cfg.N);
    fprintf(fileId, 'samples_rcp=%d\n', ...
        cfg.M * cfg.N + cfg.padLength);
    fprintf(fileId, 'samples_zp=%d\n', ...
        (cfg.M + cfg.padLength) * cfg.N);
    fprintf(fileId, 'samples_rzp=%d\n', ...
        cfg.M * cfg.N + cfg.padLength);
    fprintf(fileId, 'samples_none=%d\n', cfg.M * cfg.N);
    fprintf(fileId, 'data_cells=%d\n', nnz(masks.data));
    fprintf(fileId, 'guard_cells=%d\n', nnz(masks.guard));
    fprintf(fileId, 'observation_cells=%d\n', ...
        nnz(masks.observation));
    fprintf(fileId, 'search_cells=%d\n', nnz(masks.search));
    fprintf(fileId, 'noise_cells=%d\n', nnz(masks.noise));
end
