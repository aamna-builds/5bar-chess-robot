function fb_save(fig, name)
%FB_SAVE Save a figure as PNG into matlab/figures and close it.
    here = fileparts(mfilename('fullpath'));
    out = fullfile(here, 'figures');
    if ~exist(out, 'dir'), mkdir(out); end
    print(fig, fullfile(out, [name '.png']), '-dpng', '-r130');
    close(fig);
    fprintf('saved figures/%s.png\n', name);
end
