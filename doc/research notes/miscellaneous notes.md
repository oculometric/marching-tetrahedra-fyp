# Deformable NeRF using Recursively Subdivided Tetrahedra
link - https://dl.acm.org/doi/abs/10.1145/3664647.3681019
finds marching cubes and dense tetrahedral grids to be too computationally and memory intensive. uses recursively subdivided tetrahedra instead.
![[deformable nerf.png]]
![[tetrahedron subdivision.png]]

# Meta-Meshing and Triangulating Lattice Structures at a Large Scale
link - https://www.sciencedirect.com/science/article/pii/S0010448524000599
mentions that the interconnectivity of lattices lend poorly to parallelisation (hence independent marching/separation). they resolve this **by considering planes with interconnections**. they also use compression to overcome the memory barrier.

# Isosurface stuffing: fast tetrahedral meshes with good dihedral angles
link - https://dl.acm.org/doi/10.1145/1275808.1276448
guarantees dihedral angles of between 10.78 and 164.74 degrees. algorithms which smooth off edges are significantly faster. notes that these algorithms are rarely framerate-capable. algorithm uses octrees and BCDL as a background grid for applying a stencil (just like MC). "it only took us two days to implement" okay nerd. should take another look at this one, it seems like it could have more insights.