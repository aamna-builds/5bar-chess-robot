function xy = fb_square(P, name)
%FB_SQUARE Centre of a chess square, e.g. FB_SQUARE(P,'e4') -> [x y] mm.
%   NAME may be a char ('e4') or a cell array of names (returns N-by-2).
    if iscell(name)
        xy = zeros(numel(name), 2);
        for k = 1:numel(name), xy(k,:) = fb_square(P, name{k}); end
        return
    end
    f = double(lower(name(1))) - double('a');   % 0..7
    r = double(name(2)) - double('1');          % 0..7
    assert(f >= 0 && f <= 7 && r >= 0 && r <= 7, 'bad square %s', name);
    if strcmp(P.aSide, '+x'), col = 7 - f; else, col = f; end
    if P.rankNear == 8, row = 7 - r; else, row = r; end
    xy = [(col - 3.5) * P.sq, P.y0 + (row + 0.5) * P.sq];
end
