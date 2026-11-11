function fb_gif_append(fig, file, first, delay)
%FB_GIF_APPEND Append the current figure as a frame to an animated GIF.
%   Works in MATLAB and in GNU Octave (with the image package).
    im = frame2im(getframe(fig));
    % fixed 6x6x6 colour cube: same palette every frame, no toolbox needed
    q = uint8(round(double(im) / 51));                    % 0..5 per channel
    A = uint8(36*double(q(:,:,1)) + 6*double(q(:,:,2)) + double(q(:,:,3)));
    [r, g, b] = ndgrid(0:5, 0:5, 0:5);
    map = [r(:) g(:) b(:)];
    map = sortrows(map, [1 2 3]) / 5;
    if first
        imwrite(A, map, file, 'gif', 'LoopCount', Inf, 'DelayTime', delay);
    else
        imwrite(A, map, file, 'gif', 'WriteMode', 'append', 'DelayTime', delay);
    end
end
