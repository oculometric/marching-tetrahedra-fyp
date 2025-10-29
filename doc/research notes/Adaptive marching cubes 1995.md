link - https://link.springer.com/article/10.1007/BF01901516
grr springer grrrrr

intended to reduce the number of triangles representing the surface. solves the 'crack problem', created when neighbouring cubes are processed with different subdivision levels. this is so that the mesh can be stored more cheaply and manipulated in realtime.

basically, compute different areas at different sizes based on the curvature of the surface. recursively partition the initial cubes into smaller and smaller cubes based on the smoothness of the surface. this stops when the side length of the target cube reaches a limit, or when the surface inside the target cube is sufficiently flat (at which point it is extracted with classical MC).

crack problem occurs when neighbouring cubes are subdivided to a different degree:
![[amc crack problem.png]]
and so they just make some extra triangles to patch up the hole. pretty neat! they save a fuck ton of triangles over marching cubes (600k vs 270k type shit).