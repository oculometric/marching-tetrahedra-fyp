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