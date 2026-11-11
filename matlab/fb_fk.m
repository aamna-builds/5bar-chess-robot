function [C, ok, B1, B2] = fb_fk(P, th)
%FB_FK Forward kinematics. TH is N-by-2 motor angles [rad].
%   Returns end-effector C (N-by-2), validity OK and elbow points B1, B2.
%   The assembly mode is the one with the end effector on the far side of
%   the elbows (away from the motors), matching FB_IK.

    B1 = [P.M1(1) + P.L1*cos(th(:,1)), P.M1(2) + P.L1*sin(th(:,1))];
    B2 = [P.M2(1) + P.L1*cos(th(:,2)), P.M2(2) + P.L1*sin(th(:,2))];
    v  = B2 - B1;
    D  = sqrt(sum(v.^2, 2));
    ok = D > 1e-9 & D <= 2*P.L2;
    h  = sqrt(max(0, P.L2^2 - (D/2).^2));
    u  = v ./ D;
    n  = [-u(:,2), u(:,1)];                 % u rotated +90 deg
    C  = (B1 + B2)/2 + h .* n;
    C(~ok, :) = NaN;
end
