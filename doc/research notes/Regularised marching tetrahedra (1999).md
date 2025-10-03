link - https://www.sciencedirect.com/science/article/pii/S009784939900076X


tetrahedra actually gives worse triangle aspect ration than marching cubes.

should i consider triangle aspect ratio instead of area? or both? triangles should have roughly equal area, but also should have sensible aspect ratio.

'marching' - generate triangulation for a single cube to form a surface patch. vertices are placed where the isosurface intersects with the edges of the cube. then rinse and repeat.

MC has 256 different configurations (15 by symmetry). MC also has ambiguity problems, where the resulting geometry may contain holes (which is invalid). the solutions either introduce bias, or involve additional computation.

MT has 16 configurations (3 by symmetry). **use of the 5-tetrahedra cube tessellation reintroduces ambiguity due to the two possible ways of tesselating!!** the body-centered cubic lattice tessellation does not have this problem. this method also has better sampling efficiency.

MT overall results in more triangles.

we want regular triangles (regular area/aspect ratio) in order to achieve better looking results (both with smooth shading and with flat shading).

simplification using vertex clustering **before** triangulation. an alternative method is edge collapsing.

snapping vertices to tetrahedron corners.

> Sample points: The corners of the tetrahedral lattice, at which the data is sampled.
> Edge: A line joining two sample points contained by one tetrahedron.
> Surface intersection: The point of intersection of the iso-surface with an edge, calculated by linear interpolation of the sample points at each end of the edge.
> Vertices: The points used in the eventual triangulation of the surface.

![[body centered lattice.png]]
![[simple cubic lattice.png]]

algorithm pseudocode
```
for each sample point:
	sample the value
	for each connected edge:
		check if the value at the other end is opposite the threshold,
		and the computed intersection is closer to the current sample point:
			set a bit in the current sample point's bitfield for this edge

for each sample point:
	check if the near intersections can be merged,
	based on combinations of edge bitflags:
		compute a merged vertex position
		insert it into the list of vertices
		set the relevant edges (with near intersections)
		to reference this vertex
	else:
		compute vertex positions for each edge with a near intersection
		insert each into the list of vertices
		assign each edge a reference to the relevant vertex
		
for each tetrahedron:
	use the edge intersection flags (i.e. whether each edge has a vertex
	reference set) to determine which pattern to use
	use the lookup table to generate a triangulation, using the
	vertices specified by the references on the edges
	discard triangles with one or more of the same vertex reference.
```

i think i should be able to adapt this for both simple cubic and body centered?

important to note is the possible edge configurations which determine whether intersections can be merged. see the paper for this.

this algorithm can also be cheaply modified to disable the regularisation (i think), by simply not performing clustering.

how would we chunkify this? how would we accelerate it?
i think i can implement this MUCH faster than they did.

the authors use gradient calculation to provide normals, i think this is a waste of time.