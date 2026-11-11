function L = fb_loads(P, C, A)
%FB_LOADS Motor torque and step-rate demand at points C (N-by-2).
%   A is a struct of load assumptions (see TORQUE_ANALYSIS). Returns
%     L.tau   N-by-2 worst-case torque per motor [N m] (any drag direction)
%     L.rate  N-by-2 step rate per motor at max speed, any direction [steps/s]
    g = 9.81;
    N = size(C,1);
    [th, ok] = fb_ik(P, C);
    L.tau = nan(N,2); L.rate = nan(N,2);
    rho = A.link_kg_per_m;
    L1 = P.L1/1000; L2 = P.L2/1000;
    % effective force at the effector: magnet carriage drag + half the distal links
    F = A.mu * (A.carriage_kg*g + A.magnet_normal_N) + A.mu * rho*L2*g;   % N
    tau_fric = A.mu*rho*L1*g*L1/2 + A.mu*(rho*L2/2)*g*L1;                  % N m
    I = rho*L1^3/3 + (rho*L2/2)*L1^2;                                       % kg m^2
    for k = find(ok)'
        J  = fb_jacobian(P, C(k,:), th(k,:)) / 1000;    % m/rad
        Ji = inv(J);                                     % rad/m
        lever = sqrt(sum(J.^2, 1));                      % worst drag direction
        alpha = sqrt(sum(Ji.^2, 2))' * A.accel_m_s2;     % worst accel direction
        omega = sqrt(sum(Ji.^2, 2))' * A.speed_m_s;
        L.tau(k,:)  = F*lever + tau_fric + I*alpha;
        L.rate(k,:) = omega / (2*pi) * P.stepsPerRev;
    end
end
