function Q = fb_quality(P, C)
%FB_QUALITY Pose-quality metrics for N-by-2 points C.
%   Q.ok        reachable
%   Q.trans     transmission angle at the end effector [deg] (0/180 = type-II)
%   Q.elbow     min elbow angle of the two chains [deg]      (0/180 = type-I)
%   Q.margin    min distance (deg) of either angle from its singular value
%   Q.dex       local dexterity 1/cond(J) in [0,1]
%   Q.res       worst-case end-effector motion per microstep [mm]
%   Q.safe      ok & trans/elbow inside the configured limits

    N = size(C,1);
    [Q.trans, Q.elbow, ok, th] = fb_angles(P, C);
    Q.ok = ok; Q.th = th;
    Q.dex = nan(N,1); Q.res = nan(N,1);
    dstep = P.degPerStep * pi/180;
    for k = find(ok)'
        c = C(k,:);
        J = fb_jacobian(P, c, th(k,:));
        s = svd(J);
        Q.dex(k) = s(end) / s(1);
        Q.res(k) = max(sqrt(sum(J.^2, 1))) * dstep;
    end
    tm = min(Q.trans, 180 - Q.trans);
    em = min(Q.elbow, 180 - Q.elbow);
    Q.margin = min(tm, em);
    Q.safe = ok & tm >= P.minTrans & em >= P.minElbow;
end

