function c = fb_cmap(n)
%FB_CMAP Perceptual colormap that exists in both MATLAB and Octave.
    if nargin < 1, n = 64; end
    if exist('parula', 'file') || exist('parula', 'builtin')
        c = parula(n);
    else
        c = viridis(n);
    end
end
