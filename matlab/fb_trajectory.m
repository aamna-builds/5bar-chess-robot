function T = fb_trajectory(W, vmax, amax, dt)
%FB_TRAJECTORY Time-parametrise a polyline W (K-by-2, mm). Each segment is a
%   trapezoidal (or triangular) velocity profile that starts and ends at
%   rest, so the magnet never overshoots a corner.
%   T.t (s), T.p (M-by-2 mm), T.v (M-by-1 mm/s)
    t = 0; p = W(1,:); v = 0;
    for k = 1:size(W,1) - 1
        L = norm(W(k+1,:) - W(k,:));
        if L < eps, continue; end
        u = (W(k+1,:) - W(k,:)) / L;
        ta = vmax / amax;
        if amax * ta^2 > L                % triangular profile
            ta = sqrt(L / amax); vp = amax * ta; tc = 0;
        else
            vp = vmax; tc = (L - amax*ta^2) / vmax;
        end
        Ts = 2*ta + tc;
        tt = (dt:dt:Ts)'; if isempty(tt) || tt(end) < Ts, tt = [tt; Ts]; end
        s = zeros(size(tt)); sv = zeros(size(tt));
        for j = 1:numel(tt)
            x = tt(j);
            if x < ta
                s(j) = 0.5*amax*x^2;                     sv(j) = amax*x;
            elseif x < ta + tc
                s(j) = 0.5*amax*ta^2 + vp*(x - ta);      sv(j) = vp;
            else
                r = Ts - x;
                s(j) = L - 0.5*amax*r^2;                 sv(j) = amax*r;
            end
        end
        t = [t; t(end) + tt];             %#ok<AGROW>
        p = [p; W(k,:) + s*u];            %#ok<AGROW>
        v = [v; sv];                      %#ok<AGROW>
    end
    T.t = t; T.p = p; T.v = v;
end
