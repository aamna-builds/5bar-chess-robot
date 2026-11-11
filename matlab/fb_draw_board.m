function fb_draw_board(P, labels)
%FB_DRAW_BOARD Outline the 8x8 board (and optional square labels).
    if nargin < 2, labels = false; end
    half = 4*P.sq;
    for k = 0:8
        plot([-half half], P.y0 + k*P.sq*[1 1], '-', 'Color', [0.55 0.55 0.55]);
        plot((k*P.sq - half)*[1 1], [P.y0, P.y0 + 8*P.sq], '-', 'Color', [0.55 0.55 0.55]);
    end
    rectangle('Position', [-half, P.y0, 2*half, 8*P.sq], 'LineWidth', 1.8);
    if labels
        [xy, names] = fb_board_squares(P);
        for k = 1:64
            text(xy(k,1), xy(k,2), names{k}, 'HorizontalAlignment', 'center', ...
                'FontSize', 7, 'Color', [0.35 0.35 0.35]);
        end
    end
end
