%MOTION_DEMO Plan, check and animate real chess moves on the v2 robot.
%
%   1. e2-e4 (straight line, nothing in the way)
%   2. g8-f6 (knight: routed along the lanes between squares)
%   3. a capture: the piece on d5 is parked in the graveyard, then e4xd5
%
%   Every sample of every trajectory is passed through the IK and checked
%   against the safety limits before anything would be sent to the motors.
%
%   Output: figures/motion_demo.png, figures/motion_demo.gif
%   (Octave writes large GIFs; tools/compress_gifs.sh shrinks them.)

P = fb_params();
dt = 0.02;
g = fb_graveyard(P);
park = g(find(g(:,1) < 0, 1), :);                  % first slot on the -x (black) side

carry = { ...
    'e2-e4',            [fb_square(P,'e2'); fb_square(P,'e4')]; ...
    'g8-f6 (knight)',   fb_lane_route(P, fb_square(P,'g8'), fb_square(P,'f6')); ...
    'd5 -> graveyard',  fb_park_route(P, fb_square(P,'d5'), park); ...
    'e4xd5',            [fb_square(P,'e4'); fb_square(P,'d5')] };

% interleave magnet-off travel moves so the effector never jumps
moves = cell(0, 3);                 % name, waypoints, carrying?
pos = carry{1,2}(1,:);
for k = 1:size(carry,1)
    W = carry{k,2};
    if norm(W(1,:) - pos) > 1e-9
        moves(end+1,:) = {'travel (magnet off)', [pos; W(1,:)], false}; %#ok<SAGROW>
    end
    moves(end+1,:) = {carry{k,1}, W, true}; %#ok<SAGROW>
    pos = W(end,:);
end

allT = []; allP = []; segIdx = []; t0 = 0;
for k = 1:size(moves,1)
    T = fb_trajectory(moves{k,2}, P.vmax, P.amax, dt);
    Q = fb_quality(P, T.p);
    assert(all(Q.safe), 'move %s leaves the safe workspace', moves{k,1});
    fprintf('%-20s %5.2f s, min margin %5.1f deg, path %6.1f mm\n', moves{k,1}, ...
        T.t(end), min(Q.margin), sum(sqrt(sum(diff(T.p).^2, 2))));
    allT = [allT; t0 + T.t]; allP = [allP; T.p]; segIdx = [segIdx; k*ones(size(T.t))]; %#ok<AGROW>
    t0 = allT(end) + 0.25;                       % magnet servo settle time
end

[th, ok] = fb_ik(P, allP);
assert(all(ok));
steps = round((th*180/pi - P.homeDeg) / P.degPerStep);
rate = [zeros(1,2); abs(diff(steps)) ./ diff(allT)];

f = figure('Visible', 'off', 'Position', [50 50 1300 480]);
subplot(1,3,1); hold on; axis equal;
fb_draw_board(P, true);
cols = lines(size(carry,1)); hc = [];
for k = 1:size(moves,1)
    s = segIdx == k;
    if moves{k,3}
        hc(end+1) = plot(allP(s,1), allP(s,2), '-', 'LineWidth', 2.5, 'Color', cols(numel(hc)+1,:)); %#ok<SAGROW>
    else
        plot(allP(s,1), allP(s,2), ':', 'LineWidth', 1.2, 'Color', [0.45 0.45 0.45]);
    end
end
plot(g(:,1), g(:,2), 'o', 'Color', [0.4 0.4 0.8], 'MarkerSize', 4);
fb_draw(P, fb_square(P, 'd5'));
legend(hc, carry(:,1), 'Location', 'southoutside');
title('Planned paths'); xlim([-300 300]); ylim([-40 560]);

subplot(1,3,2); hold on; grid on;
plot(allT, th*180/pi, 'LineWidth', 1.5);
xlabel('time [s]'); ylabel('motor angle [deg]'); legend('left motor', 'right motor');
title('Joint trajectories');

subplot(1,3,3); hold on; grid on;
plot(allT, rate, 'LineWidth', 1.2);
xlabel('time [s]'); ylabel('step pulses per second');
title(sprintf('Step rate (peak %.0f /s)', max(rate(:))));
legend('left motor', 'right motor');
fb_save(f, 'motion_demo');

% ---- animation --------------------------------------------------------------
here = fileparts(mfilename('fullpath'));
gifFile = fullfile(here, 'figures', 'motion_demo.gif');
if exist(gifFile, 'file'), delete(gifFile); end
f = figure('Visible', 'off', 'Position', [50 50 440 440]);
frames = 1:5:numel(allT);
for n = frames
    clf; hold on; axis equal; xlim([-300 300]); ylim([-40 560]);
    fb_draw_board(P);
    plot(g(:,1), g(:,2), 'o', 'Color', [0.6 0.6 0.85], 'MarkerSize', 3);
    for k = 1:segIdx(n)
        s = find(segIdx == k); s = s(s <= n);
        if moves{k,3}, st = '-'; c = [0.85 0.33 0.1]; else, st = ':'; c = [0.5 0.5 0.5]; end
        plot(allP(s,1), allP(s,2), st, 'Color', c, 'LineWidth', 1.6);
    end
    fb_draw(P, allP(n,:));
    title(sprintf('%s   t = %.1f s', moves{segIdx(n),1}, allT(n)));
    set(gca, 'XTick', [], 'YTick', []);
    drawnow;
    fb_gif_append(f, gifFile, n == frames(1), 0.08);
end
close(f);
fprintf('saved figures/motion_demo.gif (%d frames)\n', numel(frames));
