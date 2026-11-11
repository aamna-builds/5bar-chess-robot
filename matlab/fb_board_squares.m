function [xy, names] = fb_board_squares(P)
%FB_BOARD_SQUARES Centres of all 64 squares and their names (a1..h8).
    names = cell(64,1); xy = zeros(64,2); k = 0;
    for r = 1:8
        for f = 1:8
            k = k + 1;
            names{k} = [char('a' + f - 1), char('0' + r)];
            xy(k,:) = fb_square(P, names{k});
        end
    end
end
