function [B1, B2] = fb_elbows(P, th)
%FB_ELBOWS Elbow joint positions for motor angles TH (N-by-2, rad).
    B1 = [P.M1(1) + P.L1*cos(th(:,1)), P.M1(2) + P.L1*sin(th(:,1))];
    B2 = [P.M2(1) + P.L1*cos(th(:,2)), P.M2(2) + P.L1*sin(th(:,2))];
end
