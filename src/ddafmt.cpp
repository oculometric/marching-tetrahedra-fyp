#include "ddafmt.h"

using namespace MTVT;

// find a point on the isosurface (multiple candidate points?)
// create a tetrahedron around that point, such that one of the vertices is outside
// for each face of the tetrahedron, create a new tetrahedron by adding a vertex
// check if that tetrahedron has any isosurface intersections
// if it doesnt, discard it
// if it does, then make it 'real' and add a new vertex inside it according to dual contouring (gradient etc)
// repeat until no more candidate tetrahedron faces exist
// adjust tetrahedra?
// actually place vertices?
// join vertices together

Mesh dynamicEvaluate(float (*sample_func)(Vector3), float threshold_value)
{
    
}