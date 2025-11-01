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

## Isosurface Extraction using On-The-Fly Delaunay Tetrahedral Grids for Gradient-Based Mesh Optimisation
link - https://dl.acm.org/doi/abs/10.1145/3730851
works by creating a cloud of sample points, each carrying a signed distance field. these points are then joined into a collection of tetrahedra using delaunay triangulation. then the geometry is built from points computed on the edges of each tetrahedron, like any other MT.
impressively efficient memory usage, grid adapts to the geometry for best results and efficiency.
guarantees **water-tightness and lack of overlaps**. this is something to talk about!
introduces directional signed distance, described with spherical harmonics. uses a 'fairness' metric to minimise bad triangles.
**specifically intended for gradient-based mesh processing, not generic fixed scalar fields**
big applications in 3D reconstruction.
should talk about how perfect sharp-point and topology preservation are not important concerns for my algorithm/problem space.

## A Comprehensive Survey of Isocontouring Methods: Applications, Limitations and Perspectives
link - https://doi.org/10.3390/a17020083
different algorithms may be better adapted for the structure of the input data. main applications are geoegraphic modelling, weather mapping, medical imaging, and computer graphics.
marching cubes, dual marching cubes, marching tetrahedra, regularised marching tetrahedra, multi-regional marching tetrahedra, surface nets, ray tracing, RT with kd-trees or octrees.

**is there a way more efficient way to store stuff in C++? if the cube centers were separated into their own array, and edges were stored per-cube?.... should we doing per-tetrahedron right from the start?**

## Multi-labeled Regularized Marching Tetrahedra Method for Implicit Geological Modeling
link - https://link.springer.com/article/10.1007/s11004-023-10075-9
looks at extending RMT for surface reconstruction for multi-layer geological interfaces. basically works by labelling each sample point with whether it is inside/outside *multiple* different implicit surfaces.
applies some extra checks during the vertex clustering to prevent topology alteration with the extra intersections, and also perform curvature weighting to keep merged vertex positions on the isosurface. clever.
expand the triangle patch lookup table to cover all the extra cases. they don't seem to be producing manifold meshes but i guess that's not the point.

they seem to work from the tetrahedron level?
they mention subdivision to recover finer details.

## Fast Isocontouring For Improved Interactivity
link - https://dl.acm.org/doi/10.5555/236226.236231
this seems useful! i want interactivity.
accelerates contouring by skipping out a lot of cells. roughly $n^{\frac{2}{3}}$, where n is the number of cells, are occupied on average. this technique is specifically looking at pre-filtering cells for surface intersections.
works by preprocessing the data to extract a set of cells for which every component of the isocontour defined by a given isovalue will intersect a cell in S. S is then sorted into a search structure according to minimum and maximum value for each cell. the isocontour can then be extracted by efficiently searching for cells in S which intersect the isosurface and then propagating tests through adjacent cells.

## Constrained Elastic SurfaceNets: Generating Smooth Models from Binary Segmented Data
link - https://www.researchgate.net/publication/226670264_Constrained_elastic_surface_nets_Generating_smooth_surfaces_from_binary_segmented_data
creates vertices on the isosurface (within individual cubes), then adjusts them to minimise the deviation from the surface. maintains small details. aims to smooth out the problems of volumetric rendering by defining a geometric surface.
each sample point (input volume element) stores a scalar value and a signed distance to the closest surface point. alternative to MC as a method for triangulating binary segmented data (i.e. data where you expect to find a surface). tries to eliminate the terracing artefacts of simple techniques, and preserve details lost by MC, and also minimise the number of triangles compared to MC.
mentions spline/parametric surface representations, where the number of control points needed to ensure detail in the final model is unknown.
![[me when i relax my nodes.png]]
1. create nodes in each data segment
2. link them together
3. relax/smooth the node positions to match the surface best, without allowing them to leave their parent segments (cubes)
each initial node can have 6 links, to its neighbouring cells.

## Molecular Surface Generation Using Marching Tetrahedra
link - https://doi.org/10.1002/(SICI)1096-987X(199808)19:11%3C1268::AID-JCC6%3E3.0.CO;2-I
uses BCDL marching tetrahedra to visualise molecular surfaces. good edge aspect ratio, only one tetrahedron shape (all hail BCDL). 

## Surface Shading in the Cuberille Environment
link - https://researchoutput.ncku.edu.tw/en/publications/surface-shading-in-the-cuberille-environment/
avoids using surface geometry because it's too expensive. due to the expensiveness of generating and rendering geometry, the algorithm focuses on simply rendering the data directly (visualisation). shades pixels according to normal of each cube face (i.e. this algorithm involves the stepped data talked about by the surface nets paper), and distance to viewer.