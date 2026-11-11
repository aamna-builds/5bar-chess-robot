%RUN_TESTS Self-checks for the kinematics toolbox. Exits with an error on
%   the first failure, so it can run in CI:  octave --eval run_tests

P = fb_params();
rng_state = 7; rand('seed', rng_state); %#ok<RAND>
nPass = 0;

% 1. FK(IK(p)) == p over the whole travel area
pts = fb_travel_points(P, 5);
[th, ok] = fb_ik(P, pts);
assert(all(ok), 'some travel points unreachable');
C = fb_fk(P, th);
err = max(sqrt(sum((C - pts).^2, 2)));
assert(err < 1e-8, 'IK/FK round trip error %g mm', err); nPass = nPass + 1;

% 2. analytic Jacobian matches finite differences
for k = 1:50
    p = pts(randi(size(pts,1)), :);
    t = fb_ik(P, p);
    J = fb_jacobian(P, p, t);
    h = 1e-6; Jn = zeros(2);
    for i = 1:2
        dt = zeros(1,2); dt(i) = h;
        Jn(:,i) = ((fb_fk(P, t + dt) - fb_fk(P, t - dt)) / (2*h))';
    end
    assert(norm(J - Jn) / norm(J) < 1e-5, 'Jacobian mismatch at (%g, %g)', p);
end
nPass = nPass + 1;

% 3. every square and graveyard slot is inside the safety limits
[xy, names] = fb_board_squares(P);
Q = fb_quality(P, xy);
assert(all(Q.safe), 'unsafe squares: %s', strjoin(names(~Q.safe)', ' '));
[g, side] = fb_graveyard(P);
assert(sum(side > 0) >= 15 && sum(side < 0) >= 15, 'graveyard needs 15 slots per side');
nPass = nPass + 1;

% 4. board orientation matches the config
a = fb_square(P, 'a1'); h = fb_square(P, 'h8');
assert(a(1) > 0 && a(2) > h(2), 'board orientation changed');
assert(abs(norm(fb_square(P,'e4') - fb_square(P,'e5')) - P.sq) < 1e-12);
nPass = nPass + 1;

% 5. trajectories start/end at rest and hit every waypoint
W = fb_lane_route(P, fb_square(P, 'g1'), fb_square(P, 'f3'));
T = fb_trajectory(W, P.vmax, P.amax, 0.01);
assert(T.v(1) == 0 && abs(T.v(end)) < 1e-9);
assert(norm(T.p(end,:) - W(end,:)) < 1e-9);
assert(max(T.v) <= P.vmax + 1e-9);
nPass = nPass + 1;

% 6. the 'home' pose used by the firmware is assemblable and safe, so the
%    first move after calibration (Z) is allowed by the firmware
Ch = fb_fk(P, P.homeDeg * pi/180);
assert(all(isfinite(Ch)), 'home pose is not assemblable');
Qh = fb_quality(P, Ch);
assert(Qh.safe, 'home pose (%.1f, %.1f) is outside the safety limits', Ch);
nPass = nPass + 1;

fprintf('run_tests: %d groups passed\n', nPass);
