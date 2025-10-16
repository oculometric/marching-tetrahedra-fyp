this required gradient analysis using desmos. fuck me
![[lattice sizes demo.png]]
```
// horizontal sample point increase - 9
// horizontal edge increase - 37

// XxY

// 1x1              -  15 sp,  50 e
// 1x2              -  24 sp,  87 e
// 1x3              -  33 sp, 124 e
// 1x4              -  42 sp, 161 e
// 1x5              -  51 sp, 198 e

// sp grad =  9 (dSP/dY)
// e grad  = 37

// 2x1              -  24 sp,  87 e
// 2x2              -  38 sp, 149 e
// 2x3              -  52 sp, 211 e
// 2x4              -  66 sp, 273 e
// 2x5              -  80 sp, 335 e

// sp grad =  14 (dSP/dY)
// e grad  = 62

// 3x1              -  33 sp, 124 e
// 3x2              -  52 sp, 211 e
// 3x3              -  71 sp, 298 e
// 3x4              -  90 sp, 385 e
// 3x5              - 109 sp, 472 e

// sp grad =  19 (dSP/dY)
// e grad  = 87

// 4x1              -  42 sp, 161 e
// 4x2              -  66 sp, 273 e
// 4x3              -  90 sp, 385 e
// 4x4              - 114 sp, 497 e
// 4x5              - 138 sp, 609 e

// sp grad =  24 (dSP/dY)
// e grad  = 112

// sp grad =  9 (dSP/dX)
// sp grad = 14 (dSP/dX)
// sp grad = 19 (dSP/dX)
// sp grad = 24 (dSP/dX)
// sp grad = 29 (dSP/dX)

// dSP/dX = 9y + 6
// dSP/dY = 9x + 6

// d(dSP/dX)/dY = 5x + 4

// 1d formula = 9x + 6

// 2d formula = 5xy + 4x + 4y + 2

// 1x4x2            -  66 sp, 273 e
// 2x4x2            - 101 sp, 452 e
// 3x4x2            - 136 sp, 631 e
// 4x4x2            - 171 sp, 810 e
// 5x4x2            - 206 sp, 989 e

// sp grad =  35 (dSP/dX)

// sp grad =  24 (dSP/dZ)
// sp grad =  35 (dSP/dZ)
// sp grad =  46 (dSP/dZ)

// off by: 9, 18, 27

// off by: -14, -11, -8, -5, -2

// 5, 2, 7
// 7, 5, 9
// 5, 0, 4

// 5xy + 4x + 4y + 2 = axy - b(xy + x + y) + c(x + y + 1) - d
//                   = axy - bxy - bx - by + cx + cy + c - d
//                   = (a - b)xy + (c - b)x + (c - b)y + (c - d)

// FINAL 3D equation (SP):
// sp(x,y,z) = 2xyz + 3(xy + xz + yz) + (x + y + z) + 1

// FINAL 3D equation (E):
// e(x,y,z) = 14xyz + 11(xy + xz + yz) + (x + y + z) + 0
```
![[sp equation desmos.png]]![[e equation desmos.png]]
basically, generated some data points using blender for the number of edges and vertices for different sized lattices. looked at the gradient (for SP and E, with respect to the changing size on each axis), and eventually arrived at the form $5xy + 4x + 4y +2$ for $z=1$. after some more experimentation, expanded this to the general form $axyz - b(xy + xz + yz) + c(x + y + z) -l$ based on the knowledge that changing the size in one dimension is analogous to changing it in another (i.e. `1x2x1` has the same SP and E count as `2x1x1` or `1x1x2`). then, rearranged the $z=1$ form to give me constraints for $a$, $b$, $c$, and $d$, so that only had to drag one slider until the values for different $z$ (not shown on the graph as this would require a 4D graph; instead look at the limited $g(...)$ and $h(...)$ test values) all matched their observed values.
this was then repeated for the SP data, and some solving of simultaneous equations was done on paper to give constraints/relations for $j$, $k$, and $l$ (by equating the result of different data points), and thus reduce the problem to modifying $i$ only via a slider until all $z$ test values matched their observed values.

this gives the final forms:
$sp(x,y,z) = 2xyz + 3(xy + xz + yz) + (x + y + z) + 1$
$e(x,y,z) = 14xyz + 11(xy + xz + yz) + (x + y + z) + 0$

for the number of sample points or edges in a lattice of a given size. interesting that $c=1$ in both cases, and that $d$ is either $1$ or $0$.