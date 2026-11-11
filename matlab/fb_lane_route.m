function W = fb_lane_route(P, a, b)
%FB_LANE_ROUTE Waypoints that carry a piece from square centre A to B (1-by-2
%   mm each) along the lanes between squares, so it never passes over the
%   centre of another square. Used for knights and blocked paths.
    d = b - a;
    s = sign(d); s(s == 0) = 1;
    c1 = a + s * P.sq/2;          % corner next to the start square
    c2 = b - s * P.sq/2;          % corner next to the target square
    W = [a; c1; c2(1) c1(2); c2; b];
    W = W([true; any(diff(W) ~= 0, 2)], :);   % drop repeated points
end
