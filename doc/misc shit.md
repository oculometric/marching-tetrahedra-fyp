no gantt charts! just do kanbans

compare body-centered-lattice vs simple cube
visual quality (user testing) vs resolution
efficiency with/without pre-computation of cell corners
explain key research sources

things to actually compare:
- perceived visual quality at different voxel resolutions ( NOT THIS )
- perceived visual quality with different vertex clustering methods (none, integrated, post-processed)
- standard deviation from average triangle area, average triangle aspect ratio
- perceived visual quality with different tessellations
- performance with different voxel resolutions
- performance with different clustering methods (as above)
- performance with different tessellations

- [ ] add some options to keep the buffers around, skip sampling pass (like if you're only changing the isovalue)

STORE THE EDGE VERTEX DATA ON THE SAMPLE POINTS! ONLY ONE SAMPLE POINT WILL KNOW ABOUT THE VERTEX ANYWAY!

to paralellise the vertex pass, just use an amortised counter to uniquely index vertices, then merge the arrays (in order) at the end.

to merge vertices, first check if only one, none, or all are set, then find vertices with the most set neighbours, and merge the neighbours into them, then try again with the remaining.

my questionnaire:
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent
do you consent please

which of these images did you prefer?
and this? 
and this?

thanks for taking part.





represent the merge-ability of edges as a graph. each link in the graph represents a pair of neighbouring edges which are both set to active (and thus can be merged). store this information in an adjacency matrix of bools

first, make a copy of the template adjacency matrix, where all edges satisfy the neighbour condition. then, go through the matrix, and logical AND each value in the table with the usability of the two edges it refers to.

this gives us a graph with some links pruned, where the only links remaining are those which satisfy the both-usable condition.

now, pick some starting points - how???

all starting points, incrementally merge blocks together, unless the result would be wrong



start at the edge with the fewest (but non-zero) unmarked, ungrouped, mergeable neighbours. set its group to zero, set it as marked, and set its minimum traversal distance to zero. traverse to its neighbours repeatedly, setting their group to zero, marking them, and setting their minimum traversal distance to one greater than the previous distance (ie zero). if they are already grouped/marked AND their minimum traversal distance is less than or equal to one, then skip them. repeat this until our current traversal distance reaches the maximum limit.
once we have no more edges we can mark, re-count how many unmarked, ungrouped, mergeable neighbours each edge has, and repeat the above (incrementing the group index each time) until all edges have been marked. an edge can only be added to a group if it 1. has not been marked, or it has been marked and the current traversal distance is less than its minimum traversal distance; and 2. the current traversal distance is less than or equal to the maximum (which i think should be 4).

i think this should work! alternatively we could pick starting points which are unmarked, ungrouped, and have an opposing edge to begin (and when these are all satisfied, pick others? idk)


first separate stuff into islands (connected regions) by traversing and marking edges as part of numbered islands, until all edges are marked.
then check each island - if it has opposing edges (using bitmask) then separate it by creating starting points at either opposing edge and traversing incrementally from them, each edge is grouped according to minimum traversal depth from a starting point (see above basically). then, or otherwise, the island/group can be merged together.