link - https://www.sciencedirect.com/science/article/abs/pii/S009784939700085X
again summarises the problem with MC: adjacent cubes may generate inconsistent surfaces with holes in due to ambiguity about which pattern should be used. MT resolves this and reduces the number of possible configuration cases per cell.
![[marching cubes ambiguity.png]]

compared cubic, body-centered, face centered, found that body-centered produced the fewest sample points and the best ratio between tetrahedral side lengths.
![[primitives comparison.png]]

smaller maximum distance between any two sampling points means a higher effective resolution.