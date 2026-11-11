%EXPORT_TABLES Write the square/graveyard lookup table for the v2 robot.
%
%   output/square_table.csv : name, x, y, motor angles, step targets from the
%   home pose, singularity margin and resolution for all 64 squares and all
%   graveyard slots. Handy for calibration: drive to a square with
%   "G x y" and compare where the magnet really is.

P = fb_params();
[xy, names] = fb_board_squares(P);
[g, side] = fb_graveyard(P);
gn = cell(size(g,1), 1);
cw = 0; cb = 0;
for k = 1:size(g,1)
    if side(k) > 0, cw = cw + 1; gn{k} = sprintf('grave_x+_%02d', cw);
    else,           cb = cb + 1; gn{k} = sprintf('grave_x-_%02d', cb); end
end
allN = [names; gn]; allXY = [xy; g];
Q = fb_quality(P, allXY);
deg = Q.th * 180/pi;
steps = round((deg - P.homeDeg) / P.degPerStep);

here = fileparts(mfilename('fullpath'));
out = fullfile(here, 'output'); if ~exist(out, 'dir'), mkdir(out); end
fid = fopen(fullfile(out, 'square_table.csv'), 'w');
fprintf(fid, 'name,x_mm,y_mm,left_deg,right_deg,left_steps,right_steps,margin_deg,mm_per_step\n');
for k = 1:numel(allN)
    fprintf(fid, '%s,%.2f,%.2f,%.3f,%.3f,%d,%d,%.1f,%.4f\n', allN{k}, allXY(k,1), allXY(k,2), ...
        deg(k,1), deg(k,2), steps(k,1), steps(k,2), Q.margin(k), Q.res(k));
end
fclose(fid);
fprintf('wrote output/square_table.csv (%d rows)\n', numel(allN));
