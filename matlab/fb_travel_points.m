function [pts, kind] = fb_travel_points(P, step)
%FB_TRAVEL_POINTS Every point a piece may visit: the whole board area
%   (square centres, lanes between squares and the board edge, sampled at
%   STEP mm, default 25) plus a graveyard column on each side.
%   KIND is 1 for board points, 2 for graveyard points.
    if nargin < 2, step = P.sq/2; end
    half = 4*P.sq;
    [X, Y] = meshgrid(-half:step:half, P.y0:step:P.y0 + 8*P.sq);
    xs = half + P.grave.xOff;
    gy = (P.y0 + P.pieceDia/2 : P.grave.pitch : P.y0 + 8*P.sq - P.pieceDia/2)';
    pts  = [X(:) Y(:); xs*ones(size(gy)) gy; -xs*ones(size(gy)) gy];
    kind = [ones(numel(X),1); 2*ones(2*numel(gy),1)];
end
