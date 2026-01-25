---

kanban-plugin: board

---

## todo

- [ ] parallelise vertex pass


## backlog

- [ ] improve merging algo - checks for degeration (concave merging) and edge-of-sample-cube merging
- [ ] save OBJs in UI


## done

**Complete**
- [x] implement vertex generation
- [x] implement triangle generation
- [x] add fbm benchmark
- [x] parallelise sampling pass
- [x] add bunny benchmark
- [x] code cleanup
- [ ] implement integrated merging algorithm
- [x] benchmark number of vertices/triangles relative to number of tetrahedra
- [x] improve benchmark to include triangles and triangle stats (area, aspect ratio incl mean and sd)
- [x] generate csv data file
- [x] fix unexpected degenerate triangles
- [x] fix incorrect merging of opposing edges (redesign merging algorithm to use island-growing system)
- [x] fix bad aspect ratios and areas
- [x] implement sample-cube edge handling
- [ ] clean up project (organise files and structure)
- [x] test vertex/index reservation for speedup
- [x] implement normal generation
- [x] wrap with graphical interface to view models live
- [x] implement alternative merging algorithm
- [x] implement simple cubic lattice structure
- [ ] clean up simple cubic
- [x] fix bunny benchmark




%% kanban:settings
```
{"kanban-plugin":"board","list-collapse":[false,false,false]}
```
%%