## On-the-fly simplification of large iso-surfaces with per-cube vertex modifiability detection
link - https://link.springer.com/article/10.1007/s12650-016-0359-5
does not store the entire extracted/simplified isosurface. incrementally extracts/simplifies as needed. closer to the 'integrated' merging method, since it avoids topology analysis/reconstruction (points out that this is slow af). cites Newman and Yi 2006 for ambiguity, and Shirley and Tuchman 1990; Van Gelder and Wilhelms 1994; Nielson and Hamann 1991 for solving it. 'Dual Contouring (DC) (Ju et al. 2002) places vertices of iso-surface dual to octree cubes, and can reproduce sharp features'. notes the difficulty of preserving topology. 'An external mesh data structure (Cignoni et al. 2003) is proposed to enable the connectivity query of large meshes. Though simplification using this data structure produces high-quality simplified meshes, the maintenance of the data structure considerably slows down the simplification.' Attali et al. 2005 present the tandem algorithm, which decimates the mesh in layers and helps to avoid bad aspect ratio triangles. because of the incremental extraction/decimation, user can control decimation ratio AND batch size, allowing balancing of memory usage/decimation quality.
![[on the fly simplification.png]]
![[simplified bunny.png]]
![[annoyingly fast simplification.png]]
does not allow the user to partially re-run the algorithm with different parameters (i.e. if the isovalue has not changed), as the entire mesh has to be regenerated.

## A COMPARISON OF MESH SIMPLIFICATION ALGORITHMS
link - https://www.sciencedirect.com/science/article/abs/pii/S0097849397000824
Heckbert and Garland tell us why we want this. different algorithms produce different quality (error compared to original mesh), different speed, and different triangle count.
![[simplification table.png]]
merging methods:
- coplanarisation - not super useful, as this isn't what we care about
- decimation - eliminating components (edges, vertices, faces) and locally retriangulating. usually preserves topology (edge flipping is of interest)
- re-tiling - adding new vertices and placing them on the old mesh, removing old vertices, and producing a re-built mesh
- iteratively performing alterations (merging, splitting, swapping) to optimise a 'quality' function
- clustering - just grouping vertices by distance and merging them into a single one. bad at preserving topology, but fast
- wavelet decomposition - mostly useful for grid-like meshes, and computationally costly
- octree heirarchical remeshing - octree-ify the mesh, then destroy/unsubdivide octree levels, and regenerate the relevant mesh at lower resolution

## External Memory Management and Simplification of Huge Meshes
link - https://ieeexplore.ieee.org/document/1260746
develop an advanced representation for large meshes, using octrees for storage. dynamic loading of chunks. maintains information about topology, enabling powerful simplification techniques which preserve topology (using the quadric error metric approach). however, most samples almost double the memory requirements of the raw mesh, and just the build time of the OEMM data structure itself is on the order of a minute per million triangles (not viable).