function P = fb_with_geometry(P, L1, L2, d, y0)
%FB_WITH_GEOMETRY Copy of P with different link lengths / spacing / offset.
    P.L1 = L1; P.L2 = L2; P.d = d; P.y0 = y0;
    P.M1 = [-d/2, 0]; P.M2 = [d/2, 0];
end
