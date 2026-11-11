%RUN_ALL Run the tests and every analysis; regenerates figures/ and output/.
%   MATLAB:      >> run_all
%   GNU Octave:  $ octave --no-gui --eval "run_all"
if exist('OCTAVE_VERSION', 'builtin')
    pkg load image;   % imwrite for the GIF
end
run_tests
analyze_workspace
optimize_layout
torque_analysis
motion_demo
export_tables
