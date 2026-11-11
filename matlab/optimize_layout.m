%OPTIMIZE_LAYOUT Choose link lengths, motor spacing and board offset.
%
%   Goal: every point a piece can visit (whole 400 x 400 mm board, the lanes
%   between squares and both graveyard columns) must be reachable and stay
%   as far as possible from both kinds of singularity.
%
%   Constraints (from the v1 build):
%     * motor spacing  d  >= 100 mm  (two geared NEMA 17 housings side by side)
%     * L1 + L2 <= 580 mm  (at most ~7 % longer than v1, so torque and
%       inertia stay close to what the geared motors already handled)
%
%   Output: figures/layout_optimisation.png, output/layout_sweep.csv and the
%   recommended geometry printed to the console.

P0 = fb_params();
L1s = 230:5:300;  L2s = 260:5:340;  ds = 100:10:200;  y0s = 40:5:170;
maxTotal = 580;

rows = zeros(0, 5);
for L1 = L1s
  for L2 = L2s
    if L1 + L2 > maxTotal, continue; end
    for d = ds
      for y0 = y0s
        P = fb_with_geometry(P0, L1, L2, d, y0);
        m = fb_min_margin(P, fb_travel_points(P));
        if isfinite(m), rows(end+1, :) = [L1 L2 d y0 m]; end %#ok<AGROW>
      end
    end
  end
end

[~, best] = max(rows(:,5));
B = rows(best, :);
here = fileparts(mfilename('fullpath'));
outDir = fullfile(here, 'output'); if ~exist(outDir, 'dir'), mkdir(outDir); end
fid = fopen(fullfile(outDir, 'layout_sweep.csv'), 'w');
fprintf(fid, 'L1_mm,L2_mm,d_mm,y0_mm,min_margin_deg\n');
fprintf(fid, '%g,%g,%g,%g,%.3f\n', rows');
fclose(fid);

v1 = fb_params(fullfile(here, '..', 'config', 'robot_v1.json'));
pts1 = fb_travel_points(v1);
[t1, e1, ok1] = fb_angles(v1, pts1);
m1 = min([min(t1(ok1), 180 - t1(ok1)); min(e1(ok1), 180 - e1(ok1))]);

fprintf('\n== Layout optimisation ==\n');
fprintf('feasible layouts evaluated : %d\n', size(rows,1));
fprintf('v1 (as built)  L1=%g L2=%g d=%g y0=%g  -> %d of %d travel points out of reach, min margin on the rest %5.2f deg\n', ...
        v1.L1, v1.L2, v1.d, v1.y0, sum(~ok1), numel(ok1), m1);
fprintf('v2 (optimised) L1=%g L2=%g d=%g y0=%g  -> min margin %6.2f deg\n', B);

% Pareto front: best margin for each total link length
tot = rows(:,1) + rows(:,2);
T = unique(tot);
front = arrayfun(@(t) max(rows(tot <= t, 5)), T);

f = figure('Visible', 'off', 'Position', [100 100 900 420]);
subplot(1,2,1);
plot(T, front, '-o', 'LineWidth', 1.5, 'MarkerSize', 4); hold on; grid on;
plot(B(1)+B(2), B(5), 'p', 'MarkerSize', 16, 'MarkerFaceColor', [0.85 0.33 0.1]);
plot(v1.L1 + v1.L2, m1, 's', 'MarkerSize', 10, 'MarkerFaceColor', [0.5 0.5 0.5]);
xlabel('L1 + L2  [mm]'); ylabel('worst-case singularity margin [deg]');
title('Best achievable margin vs. total link length');
legend('Pareto front', 'v2 choice', 'v1 as built (corners out of reach)', 'Location', 'southeast');

subplot(1,2,2);
sel = rows(:,1) == B(1) & rows(:,2) == B(2);
S = rows(sel, :);
dd = unique(S(:,3)); yy = unique(S(:,4));
M = nan(numel(yy), numel(dd));
for k = 1:size(S,1)
    M(yy == S(k,4), dd == S(k,3)) = S(k,5);
end
shown = M; shown(isnan(M)) = -2;               % infeasible -> grey
imagesc(dd, yy, shown); axis xy; hold on;
colormap([0.82 0.82 0.82; fb_cmap(63)]); caxis([-2 max(M(:))]);
cb = colorbar; ylabel(cb, 'margin [deg]  (grey = corners out of reach)');
plot(B(3), B(4), 'wp', 'MarkerSize', 16, 'MarkerFaceColor', 'w');
xlabel('motor spacing d [mm]'); ylabel('board near-edge offset y_0 [mm]');
title(sprintf('Margin map at L1=%g, L2=%g', B(1), B(2)));
fb_save(f, 'layout_optimisation');
