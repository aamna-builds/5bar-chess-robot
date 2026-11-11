function [m, allReach] = fb_min_margin(P, pts)
%FB_MIN_MARGIN Smallest singularity margin [deg] over a set of points.
%   Returns -Inf (and allReach=false) if any point is out of reach.
    [t, e, ok] = fb_angles(P, pts);
    allReach = all(ok);
    if ~allReach, m = -Inf; return; end
    m = min([min(t, 180 - t); min(e, 180 - e)]);
end
