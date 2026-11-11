function P = fb_params(cfgFile)
%FB_PARAMS Load robot parameters from config/robot.json.
%   P = FB_PARAMS() reads the default config next to this repository.
%   P = FB_PARAMS(FILE) reads a specific JSON file.
%
%   Frame: origin midway between the two motor shafts, +x to the right,
%   +y away from the motors (towards the board). Lengths in mm.

    if nargin < 1
        here = fileparts(mfilename('fullpath'));
        cfgFile = fullfile(here, '..', 'config', 'robot.json');
    end
    txt = fileread(cfgFile);
    C = jsondecode(txt);

    P.L1 = C.geometry.proximal_mm;          % motor -> elbow
    P.L2 = C.geometry.distal_mm;            % elbow -> end effector
    P.d  = C.geometry.motor_spacing_mm;     % shaft to shaft
    P.M1 = [-P.d/2, 0];                     % left motor
    P.M2 = [ P.d/2, 0];                     % right motor

    P.sq         = C.board.square_mm;
    P.y0         = C.board.near_edge_y_mm;
    P.rankNear   = C.board.rank_nearest_motors;
    P.aSide      = C.board.a_file_side;
    P.pieceDia   = C.piece.base_diameter_mm;

    P.grave.xOff  = C.graveyard.x_offset_from_board_edge_mm;
    P.grave.pitch = C.graveyard.pitch_mm;

    P.stepsPerRev = C.drive.full_steps_per_rev * C.drive.microsteps * C.drive.gear_ratio;
    P.degPerStep  = 360 / P.stepsPerRev;
    P.homeDeg     = C.drive.home_deg(:)';

    P.vmax  = C.motion.max_speed_mm_s;
    P.amax  = C.motion.accel_mm_s2;
    P.seg   = C.motion.segment_mm;

    P.minTrans = C.safety.min_transmission_deg;
    P.minElbow = C.safety.min_elbow_deg;
    P.raw = C;
end
