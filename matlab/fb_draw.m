function h = fb_draw(P, C, varargin)
%FB_DRAW Draw the linkage with its end effector at C (1-by-2). Returns handles.
    opts = struct('color', [0.15 0.15 0.15], 'lw', 3);
    for k = 1:2:numel(varargin), opts.(varargin{k}) = varargin{k+1}; end
    th = fb_ik(P, C);
    [B1, B2] = fb_elbows(P, th);
    pts = [P.M1; B1; C; B2; P.M2];
    h(1) = plot(pts(:,1), pts(:,2), '-', 'Color', opts.color, 'LineWidth', opts.lw);
    h(2) = plot(pts(:,1), pts(:,2), 'o', 'MarkerSize', 6, ...
        'MarkerFaceColor', 'w', 'MarkerEdgeColor', opts.color, 'LineWidth', 1.5);
    h(3) = plot([P.M1(1) P.M2(1)], [0 0], 's', 'MarkerSize', 11, ...
        'MarkerFaceColor', [0.3 0.3 0.3], 'MarkerEdgeColor', 'k');
    h(4) = plot(C(1), C(2), 'o', 'MarkerSize', 9, 'MarkerFaceColor', [0.85 0.2 0.2], ...
        'MarkerEdgeColor', 'k');
end
