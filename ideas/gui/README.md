# A GUI for ceps

Longstanding idea, now ripe for implementation. 

## Treemap of states
State machines are hierarchical which make treemap a well suited form of graphical representation. A State machine S is represented by a grid of cells a cell is a leaf in the case it represents an atomic state, otherwise it is the treemap of the corresponding sub state machine, the area occupied by a tree map is determined by the area use for a single leaf times the number of atomic states contained transitively in the corresponding state machine. An active state has a different rendering (e.g. different color) than an inactive state, a composite state is active if one of its sub states is.
## Extension to edges
In addition to states the transitions need some place, first of all they are optional, so the simplest graphical representation is the treemap described in the previous section. The simplest extension is to draw states with enough border space so that tranisitons adjacent to two adjacent states in the treemap representation can be easily drawn horizontally/vertically without crossing a leaf, crossover-edges are obviously difficult to handle, there is the obvious possiblity of a lines which bend in multiples of 90° using the grid space to reach the target state, this can be pretty messy, but it is easy to implement and easy to understand.
## The Code
Assumption: Raylib 5+ sibling directory of ceps.
