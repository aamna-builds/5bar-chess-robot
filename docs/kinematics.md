# Kinematics of the 5-bar linkage

The same equations are implemented three times, and the three versions are tested against each other:

- [`matlab/fb_ik.m`](../matlab/fb_ik.m), [`fb_fk.m`](../matlab/fb_fk.m) and [`fb_jacobian.m`](../matlab/fb_jacobian.m) are used for analysis and design.
- [`host/fivebar_chess/kinematics.py`](../host/fivebar_chess/kinematics.py) is used for planning.
- [`firmware/fivebar_chess/kinematics.cpp`](../firmware/fivebar_chess/kinematics.cpp) runs on the robot.

## Frame and names

```
                 C  (end effector, magnet)
                / \
          L2   /   \   L2         distal links
              /     \
            B1       B2           elbows (passive joints)
             \       /
         L1   \     /   L1        proximal links
               \   /
        M1 ●----- ● M2            motors, distance d apart
           (-d/2,0)  (d/2,0)
```

- The origin is midway between the motor shafts. +x points right and +y points towards the board.
- θ₁ and θ₂ are the motor angles, measured counter-clockwise from +x.
- The v2 values are L1 = 265 mm, L2 = 315 mm and d = 100 mm. The board's near edge is at y₀ = 120 mm. All values live in [`config/robot.json`](../config/robot.json).

## Degrees of freedom

Gruebler's equation for a planar mechanism with n = 5 links (ground included) and j = 5 revolute joints:

$$M = 3(n-1) - 2j = 3\cdot 4 - 2\cdot 5 = 2$$

Two motors give two degrees of freedom, which is exactly the x–y motion the magnet needs. The vertical "pick" is done by a servo that flips the magnet towards or away from the board, so it never enters the planar kinematics.

## Inverse kinematics (position → motor angles)

Each side is a two-link chain. For the left motor:

$$r_1 = \lVert C - M_1 \rVert,\qquad
\theta_1 = \operatorname{atan2}(C_y - M_{1y},\; C_x - M_{1x}) + \arccos\!\frac{L_1^2 + r_1^2 - L_2^2}{2L_1 r_1}$$

For the right motor, the sign in front of the arccos is negative. Each chain therefore has two elbow solutions, giving four *working modes* in total. The robot uses **elbows-out**: the left elbow swings left and the right elbow swings right. This keeps the links apart and gives the best-conditioned workspace above the motors.

A point is reachable only if both arccos arguments lie in [−1, 1], which means |L1 − L2| ≤ rᵢ ≤ L1 + L2.

## Forward kinematics (motor angles → position)

$$B_i = M_i + L_1(\cos\theta_i, \sin\theta_i)$$

C is one of the two intersections of the circles of radius L2 around B₁ and B₂. With u the unit vector from B₁ to B₂ and n the vector u rotated by +90°:

$$C = \tfrac{1}{2}(B_1 + B_2) + \sqrt{L_2^2 - \tfrac14\lVert B_2 - B_1\rVert^2}\; n$$

The + sign picks the *assembly mode* with the effector on the far side of the elbows. This is the mode the robot is built in.

## Velocity Jacobian and singularities

Differentiating the loop-closure constraints ‖C − Bᵢ‖² = L₂² gives

$$\underbrace{\begin{bmatrix}(C-B_1)^T\\(C-B_2)^T\end{bmatrix}}_{A}\,\dot C =
\underbrace{\operatorname{diag}\big((C-B_i)\cdot \tfrac{\partial B_i}{\partial\theta_i}\big)}_{B}\,\dot\theta,
\qquad J = A^{-1}B$$

There are two kinds of singularity:

| Type | Condition | Geometry | What happens |
|---|---|---|---|
| I (inverse) | det B = 0 | a proximal and a distal link are aligned (elbow angle 0° or 180°) | edge of the reachable area; one direction cannot be reached |
| II (direct) | det A = 0 | the two distal links are aligned (transmission angle 0° or 180°) | the effector can move with both motors locked; torque demand → ∞ |

The robot tracks two angles in degrees, because they are easy to picture and cheap to compute on the Arduino:

- The **transmission angle** is the angle between the two distal links at C.
- The **elbow angle** is the angle between the proximal and distal link at each elbow.

The *margin* is the smaller distance of either angle from 0° or 180°. v2 requires a margin of at least 20° everywhere a piece can go: on the squares, on the lanes between squares and on the graveyard slots. The firmware rejects any move that would leave this region.

## Why v1 struggled near the motors

On the as-built v1 geometry (L1 = 250, L2 = 290, d = 165, y₀ = 62.5 mm), the ranks nearest the motors sit almost on a type-II singularity:

- **Square c8** is only 0.4° from the singularity. A microstep moves the magnet up to **1.4 mm** there, compared with about 0.02 mm elsewhere.
- **Pushing a piece** on those ranks would need tens of N·m in the worst direction.
- **The two far board corners** are 2 mm out of reach.

These numbers match what was observed in testing:

- "unpredictable motion" and "loss of control" in certain arm positions
- reduced accuracy at the edges of the board
- step loss

See [`matlab/figures/workspace_v1_vs_v2.png`](../matlab/figures/workspace_v1_vs_v2.png) and [`torque_and_speed.png`](../matlab/figures/torque_and_speed.png).

## Resolution

With 200 steps/rev, 8× microstepping and a 14:1 gearbox, one microstep turns the output shaft by 360°/22,400 = 0.016°. The size of that step at the magnet depends on the Jacobian. On v2 it is at most **0.08 mm** anywhere on the board, which is far finer than any mechanical play in the joints.
