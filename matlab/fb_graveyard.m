function [slots, sideOf] = fb_graveyard(P)
%FB_GRAVEYARD Parking slots for captured pieces, one column each side.
%   SLOTS is K-by-2, SIDEOF is K-by-1 (+1 right/+x, -1 left/-x). Only slots
%   inside the safe workspace are returned, nearest-the-motors first.
    xs = P.sq*4 + P.grave.xOff;
    ys = (P.y0 + P.pieceDia/2 : P.grave.pitch : P.y0 + 8*P.sq - P.pieceDia/2)';
    cand = [ [ xs*ones(size(ys)), ys]; [-xs*ones(size(ys)), ys] ];
    side = [ones(size(ys)); -ones(size(ys))];
    Q = fb_quality(P, cand);
    slots = cand(Q.safe, :);
    sideOf = side(Q.safe);
end
