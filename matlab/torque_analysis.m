%TORQUE_ANALYSIS Motor torque and step-rate demand over the board.
%
%   Static/quasi-static model, worst case over the direction of motion:
%       tau_i = |J(:,i)| * F  +  tau_friction  +  I * alpha_i
%   J is the velocity Jacobian (m/rad). Near a type-II singularity |J| grows
%   without bound, so the torque needed to push the magnet carriage there
%   grows without bound too. This is why the v1 layout lost steps and
%   behaved unpredictably on the ranks closest to the motors.
%
%   Panel 3 shows the per-rank peak against the motor budgets: the bare
%   motor is not enough anywhere (hence the planetary gearbox), and on v1
%   even the gearbox is exceeded on the ranks nearest the motors.
%
%   All load numbers below are ASSUMPTIONS (estimates, not measurements).
%   Edit them and re-run to see the effect.

A.mu              = 0.30;    % acrylic-on-acrylic sliding friction
A.carriage_kg     = 0.06;    % magnet carriage + servo
A.magnet_normal_N = 4.0;     % magnet pulling carriage against the board
A.link_kg_per_m   = 0.12;    % 25 x 4 mm acrylic link
A.speed_m_s       = 0.12;    % = motion.max_speed_mm_s
A.accel_m_s2      = 0.40;    % = motion.accel_mm_s2

motor.bare_Nm    = 0.25;     % NEMA 17 usable pull-out torque at speed (bare)
motor.geared_Nm  = 4.4;      % 45 kg cm rated output of the planetary gearbox
motor.max_rate   = 20000;    % step pulses/s the firmware can generate per motor

here = fileparts(mfilename('fullpath'));
V1 = fb_params(fullfile(here, '..', 'config', 'robot_v1.json'));
V2 = fb_params(fullfile(here, '..', 'config', 'robot.json'));

f = figure('Visible', 'off', 'Position', [50 50 1400 470]);
designs = {V1, V2}; names = {'v1 as built', 'v2 optimised'};
fprintf('\n== Torque and speed demand (assumed loads) ==\n');
for i = 1:2
    P = designs{i};
    [X, Y] = meshgrid(-200:10:200, P.y0:10:P.y0 + 400);
    Lp = fb_loads(P, [X(:) Y(:)], A);
    T = reshape(max(Lp.tau, [], 2), size(X));
    subplot(1, 3, i); hold on;
    imagesc(X(1,:), Y(:,1), log10(T)); axis xy equal tight;
    colormap(fb_cmap(64)); caxis(log10([0.01 2]));
    fb_draw_board(P);
    cb = colorbar;
    tk = log10([0.01 0.03 0.1 0.3 1 2]);
    set(cb, 'YTick', tk, 'YTickLabel', {'0.01','0.03','0.1','0.3','1','2'});
    ylabel(cb, 'peak motor torque [N m] (log scale)');
    title(sprintf('%s: worst %.2f N m', names{i}, max(T(:))));
    xlabel('x [mm]'); ylabel('y [mm]');
    [sq, sqn] = fb_board_squares(P);
    Ls = fb_loads(P, sq, A);
    fprintf('%-13s peak torque on board %.3f N m (squares only %.3f N m), peak step rate %.0f /s\n', ...
        names{i}, max(T(:)), max(Ls.tau(:)), max(Lp.rate(:)));
end

% peak torque per rank: where on the board does each design struggle?
subplot(1, 3, 3); hold on; grid on;
mk = {'-o', '-s'};
for i = 1:2
    P = designs{i};
    peakRank = zeros(1, 8);
    for r = 1:8
        sqs = arrayfun(@(f) sprintf('%c%d', 'a' + f - 1, r), 1:8, 'UniformOutput', false);
        Lr = fb_loads(P, fb_square(P, sqs), A);
        peakRank(r) = max(Lr.tau(:));
    end
    semilogy(1:8, peakRank, mk{i}, 'LineWidth', 1.6, 'MarkerSize', 6);
end
semilogy([1 8], motor.bare_Nm*[1 1], '--', 'LineWidth', 1.3);
semilogy([1 8], motor.geared_Nm*[1 1], '--', 'LineWidth', 1.3);
set(gca, 'YScale', 'log'); ylim([0.05 100]); xlim([0.8 8.2]);
xlabel('rank (8 = closest to the motors)'); ylabel('peak torque on that rank [N m]');
title('Peak torque per rank');
legend('v1 as built', 'v2 optimised', 'bare NEMA 17', 'NEMA 17 + gearbox', 'Location', 'northwest');
fb_save(f, 'torque_and_speed');

fprintf('bare NEMA 17 budget %.2f N m, geared %.1f N m, firmware step budget %d /s\n', ...
    motor.bare_Nm, motor.geared_Nm, motor.max_rate);
