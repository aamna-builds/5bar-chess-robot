function [th, ok] = fb_ik(P, C)
%FB_IK Inverse kinematics of the 5-bar linkage (elbows-out working mode).
%   [TH, OK] = FB_IK(P, C) where C is N-by-2 end-effector positions [mm].
%   TH is N-by-2 motor angles [rad], measured CCW from +x.
%   OK is N-by-1 logical, false where the point is out of reach.

    x = C(:,1); y = C(:,2);
    [a1, g1, ok1] = side(x - P.M1(1), y - P.M1(2), P.L1, P.L2);
    [a2, g2, ok2] = side(x - P.M2(1), y - P.M2(2), P.L1, P.L2);
    th = [a1 + g1, a2 - g2];       % left elbow swings out left, right elbow out right
    ok = ok1 & ok2;
    th(~ok, :) = NaN;
end

function [a, g, ok] = side(dx, dy, L1, L2)
    r  = sqrt(dx.^2 + dy.^2);
    c  = (L1^2 + r.^2 - L2^2) ./ (2 * L1 * r);
    ok = (r > 1e-9) & (abs(c) <= 1);
    c  = max(-1, min(1, c));
    a  = atan2(dy, dx);
    g  = acos(c);
end
