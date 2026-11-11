function W = fb_park_route(P, from, slot)
%FB_PARK_ROUTE Lane route from a square centre to a graveyard slot: step to
%   a square corner, run along the lane to the board edge, then along the
%   outside of the board to the slot.
    sx = sign(slot(1) - from(1)); if sx == 0, sx = 1; end
    sy = sign(slot(2) - from(2)); if sy == 0, sy = 1; end
    c1   = from + [sx sy] * P.sq/2;
    edge = [sx * 4*P.sq, c1(2)];
    W = [from; c1; edge; edge(1), slot(2); slot];
    W = W([true; any(abs(diff(W)) > 1e-9, 2)], :);
end
