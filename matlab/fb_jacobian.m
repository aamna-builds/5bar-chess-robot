function [J, A, Bd] = fb_jacobian(P, C, th)
%FB_JACOBIAN Velocity Jacobian at one pose: dC = J * dtheta.
%   C is 1-by-2, TH is 1-by-2 (from FB_IK). Loop-closure constraints
%   |C - B_i|^2 = L2^2 differentiate to  A*dC = Bd*dtheta  with
%     A  = [ (C-B1) ; (C-B2) ]            (type-II singular when det(A)=0)
%     Bd = diag((C-B_i) . dB_i/dtheta_i)  (type-I  singular when det(Bd)=0)

    [B1, B2] = fb_elbows(P, th);
    dB1 = P.L1 * [-sin(th(1)), cos(th(1))];
    dB2 = P.L1 * [-sin(th(2)), cos(th(2))];
    A  = [C - B1; C - B2];
    Bd = diag([dot(C - B1, dB1), dot(C - B2, dB2)]);
    J  = A \ Bd;
end
