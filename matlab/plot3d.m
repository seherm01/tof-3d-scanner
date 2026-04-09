
% Seher Mian

clear; clc;

% Configuration
port = "COM7";
baudrate = 115200;
pointsPerScan = 32;

% Serial setup
device = serialport(port, baudrate);
device.Timeout = 200;

configureTerminator(device, "LF");
flush(device);

fprintf("Opened %s\n", port);
% Display on command window
disp("Press PJ0 to start, PJ1 for more scans, PJ0 again to finish...");

%-Store data
X = []; Y = []; Z = [];
C = [];    % scan index
A = [];    % angle index

scan = 0;

% Main loop
while true
    raw = readline(device);
  
    if isempty(raw)
        continue;
    end

    line = strtrim(char(raw));
    disp("Received: " + line);

    % Control messages (based on pushbuttons)
    if contains(line, "START_SCAN")
        scan = scan + 1;
        fprintf("\n--- Scan %d START ---\n", scan);
        continue;
    end

    if contains(line, "END_SCAN")
        fprintf("Scan %d complete.\n", scan);
        continue;
    end

    if contains(line, "DONE")
        disp("All scans complete.");
        break;
    end

    % Parse data 
    vals = sscanf(line, '%f,%f,%f,%f,%f,%f');
    if numel(vals) ~= 6
        continue;
    end

    angleNum    = vals(1);
    rawDistance = vals(2);

    % Convert to cm
    distance = rawDistance / 10;

    % Convert polar to cartesian
    theta = (angleNum - 1) * (2*pi / pointsPerScan);

    x = (scan - 1) * 40;   % spacing between scans (cm)[change based on use]
    y = distance * cos(theta);
    z = distance * sin(theta);

    % Store data
    X(end+1) = x;
    Y(end+1) = y;
    Z(end+1) = z;
    C(end+1) = scan;
    A(end+1) = angleNum;

    fprintf("Scan %d Point %d -> (%.2f, %.2f, %.2f)\n", ...
            scan, angleNum, x, y, z);
end

clear device;

% Plot
numPositions = max(C);
xPositions = 0:30:(30*(numPositions-1));
cmap = jet(numPositions);

figure;

% Scatter plot (using scatter3)
subplot(1,2,1)
scatter3(X, Y, Z, 60, C, 'filled')
colormap(cmap)
colorbar
xlabel('X (cm)')
ylabel('Y (cm)')
zlabel('Z (cm)')
title('3D Scatter')
grid on
axis equal

% Scan rings
subplot(1,2,2)
hold on

for p = 1:numPositions
    idx = find(C == p);
    if isempty(idx), continue; end

    % Sort by angle
    [~, s] = sort(A(idx));
    idx = idx(s);

    % Close loop
    yLoop = [Y(idx), Y(idx(1))];
    zLoop = [Z(idx), Z(idx(1))];
    xLoop = repmat(xPositions(p), 1, length(yLoop));

    % Plot ring (plot3)
    plot3(xLoop, yLoop, zLoop, '-o', ...
        'Color', cmap(p,:), ...
        'LineWidth', 2, ...
        'MarkerFaceColor', cmap(p,:));

    % Connect to previous scan
    if p > 1
        prevIdx = find(C == p-1);
        [~, s2] = sort(A(prevIdx));
        prevIdx = prevIdx(s2);

        n = min(length(prevIdx), length(idx));

        for k = 1:n
            plot3([xPositions(p-1), xPositions(p)], ...
                  [Y(prevIdx(k)), Y(idx(k))], ...
                  [Z(prevIdx(k)), Z(idx(k))], ...
                  '-', 'Color', [0.7 0.7 0.7]);
        end
    end
end

xlabel('X (cm)')
ylabel('Y (cm)')
zlabel('Z (cm)')
title('Scan Planes Connected')
grid on
axis equal
view(35,25)
hold off
