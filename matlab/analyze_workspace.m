%ANALYZE_WORKSPACE Reachability and singularity margin, v1 (as built) vs v2.
%
%   For every point in the plane the script computes the inverse kinematics
%   and the two singularity indicators:
%     * transmission angle between the distal links (type-II singularity at
%       0 / 180 deg: the effector can move while both motors are held)
%     * elbow angle between proximal and distal link (type-I singularity at
%       0 / 180 deg: edge of the reachable area)
%   The colour shows the smaller distance (deg) to either singularity.
%
%   Output: figures/workspace_v1_vs_v2.png and a console summary per square.

here = fileparts(mfilename('fullpath'));
cfgs = {fullfile(here, '..', 'config', 'robot_v1.json'), fullfile(here, '..', 'config', 'robot.json')};
titles = {'v1 - as built (April 2026)', 'v2 - optimised layout'};

Pref = fb_params(cfgs{2});      % judge both designs by the v2 safety limits
f = figure('Visible', 'off', 'Position', [50 50 1250 640]);
for i = 1:2
    P = fb_params(cfgs{i});
    P.minTrans = Pref.minTrans; P.minElbow = Pref.minElbow;
    [X, Y] = meshgrid(-380:4:380, -40:4:700);
    [t, e] = fb_angles(P, [X(:) Y(:)]);
    M = reshape(min(min(t, 180 - t), min(e, 180 - e)), size(X));

    subplot(1, 2, i); hold on;
    shown = M; shown(isnan(M)) = -3;
    imagesc(X(1,:), Y(:,1), shown); axis xy equal tight;
    colormap([0.9 0.9 0.9; fb_cmap(63)]); caxis([-3 60]);
    fb_draw_board(P);

    % squares coloured by status
    [xy, names] = fb_board_squares(P);
    Q = fb_quality(P, xy);
    plot(xy(Q.safe,1),  xy(Q.safe,2),  '.', 'Color', [0.1 0.6 0.2], 'MarkerSize', 12);
    plot(xy(~Q.safe,1), xy(~Q.safe,2), 'x', 'Color', [0.85 0.1 0.1], 'MarkerSize', 9, 'LineWidth', 2);

    pts = fb_travel_points(P);
    [~, ~, okAll] = fb_angles(P, pts);
    plot(pts(~okAll,1), pts(~okAll,2), 'rp', 'MarkerSize', 13, 'MarkerFaceColor', 'r');

    [g, ~] = fb_graveyard(P);
    plot(g(:,1), g(:,2), 'o', 'MarkerSize', 5, 'Color', [0.2 0.2 0.8]);

    fb_draw(P, fb_square(P, 'd4'));
    title(sprintf('%s\nL1=%g  L2=%g  d=%g  y0=%g mm', titles{i}, P.L1, P.L2, P.d, P.y0));
    xlim([-380 380]); ylim([-60 650]);
    xlabel('x [mm]'); ylabel('y [mm]');
    cb = colorbar; ylabel(cb, 'distance to nearest singularity [deg]');

    fprintf('\n== %s ==\n', titles{i});
    fprintf('squares inside v2 limits     : %d / 64\n', sum(Q.safe));
    fprintf('worst square                 : %s (margin %.1f deg)\n', names{find(Q.margin == min(Q.margin), 1)}, min(Q.margin));
    fprintf('travel points out of reach   : %d\n', sum(~okAll));
    fprintf('graveyard slots (right/left) : %d / %d\n', sum(g(:,1) > 0), sum(g(:,1) < 0));
    fprintf('worst dexterity 1/cond(J)    : %.3f\n', min(Q.dex));
    fprintf('coarsest resolution          : %.4f mm per microstep\n', max(Q.res));
    [thT, okT] = fb_ik(P, pts(okAll,:));
    [E1, E2] = fb_elbows(P, thT);
    fprintf('elbow sweep envelope         : x %.0f..%.0f mm, y %.0f..%.0f mm (keep clear)\n', ...
        min([E1(:,1); E2(:,1)]), max([E1(:,1); E2(:,1)]), min([E1(:,2); E2(:,2)]), max([E1(:,2); E2(:,2)]));
end
fb_save(f, 'workspace_v1_vs_v2');
