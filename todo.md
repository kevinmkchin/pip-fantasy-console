# todo

### Language features

- tables (require gc)
    - index tables with strings <
    - add relops for GCObject type TValues. for other ops, just crash? 
    - only increment refcount for gc obj upon assignment to a variable. that means, if we create a table, we don't auto set refcount to 1, and also must instantly delete if we don't assign to a variable at all.
    - release GC reference if lose reference, and then propagate to element gc objects
    
    - need way to delete a table entry <- could be a standard library function
    - actual array implementation <- MAYBE JUST MAKE AN ENTIRE NEW ARRAY TYPE??? THAT MIGHT BE SO MUCH CLEANER AND NICER -> is there ANY reason to use a table as also an array?

    - initialize array elements like so { 0, 1, c, 3, "e" }
    - initialize table elements like so { 0 = 3, m = y, "e" = x }

- strings


- while loops & break
- for loops
- floats
- elifs

Way the fuck down the line:
- use custom assert for mesascript
- replace all std data structs with alternate optimized implementations in C?
- REFACTOR


### Uncategorized

- GUI incorrect zones when window resize
  - prob cuz mouse x y will be incorrect.
- Make sure vertext height thing didn't regress (text height inconsistency for loading vs forming vertices)
- Game GUI atomics
- Editor camera & game camera
- Pixel perfect AABB Collision checks -> GJK&EPA
  - Select collider type from AABB, Sphere, and Convex point cloud
- Sprite sheet & animations
- Sprite batch rendering
- Game file management
  - meta data (Title, Cover art, Author, etc.)
  - everything needed to play or edit the game
- Screen system
- Object system
- Script interpreter

Missing from Cute or Ascent
- ECS + components
- Asset management
- Console















NOTES:

potentially:
auto
const
float
int
bool
string
struct
void
enum
break
continue
for
while
do
switch
case
default


I want:
either
- boxes instead of pointers AND structs or arrays
OR
- tables that are always reference type


----
- boxes instead of pointers
    - boxes are reference pointers to a copy of a value that exists in that scope only?
        e.g. x = 4; box b = box(x); x = 7; x != unbox(b);
        setbox(b, unbox(b) + 2);
- tables instead of structs?
- vectors or arrays or something
- hashtable? or dictionary?




