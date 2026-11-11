function [trans, elbow, ok, th] = fb_angles(P, C)
%FB_ANGLES Vectorised transmission and elbow angles [deg] for points C (N-by-2).
%   TRANS is the angle between the two distal links at the end effector
%   (0 or 180 deg = type-II / "direct" singularity: the effector can move
%   with the motors locked). ELBOW is the smaller of the two angles between
%   proximal and distal link (0 or 180 deg = type-I / "inverse" singularity:
%   edge of the reachable workspace).
    [th, ok] = fb_ik(P, C);
    [B1, B2] = fb_elbows(P, th);
    trans = ang(B1 - C, B2 - C);
    e1 = ang(P.M1 - B1, C - B1);
    e2 = ang(P.M2 - B2, C - B2);
    elbow = min(e1, e2);
end

function a = ang(u, v)
    a = atan2(abs(u(:,1).*v(:,2) - u(:,2).*v(:,1)), sum(u.*v, 2)) * 180/pi;
end
