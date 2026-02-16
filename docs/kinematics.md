# 5-Bar Linkage Kinematics

## Degrees of Freedom (DOF)

Using Gruebler’s equation for planar mechanisms:

M = 3(n - 1) - 2j

Where:
n = number of links = 5  
j = number of revolute joints = 5  

M = 3(5 - 1) - 2(5)  
M = 12 - 10  
M = 2 DOF  

The mechanism has 2 degrees of freedom.

---

## Geometry Parameters

Motor spacing (d) = 360 mm  
Primary link (L1) = 220 mm  
Secondary link (L2) = 220 mm  

Left motor at (0,0)  
Right motor at (d,0)

End effector position = (x, y)

---

## Inverse Kinematics

For left motor:

r1 = sqrt(x² + y²)

θ1 = atan2(y, x) ± acos((L1² + r1² - L2²) / (2L1r1))

For right motor:

r2 = sqrt((x - d)² + y²)

θ2 = atan2(y, x - d) ∓ acos((L1² + r2² - L2²) / (2L1r2))

Elbow-down configuration will be selected for stability.
