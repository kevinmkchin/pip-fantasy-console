# todo

### Done

- index tables with strings
- initialize array elements
- sort of lists/arrays - actual array implementation <- MAYBE JUST MAKE AN ENTIRE NEW ARRAY TYPE??? THAT MIGHT BE SO MUCH CLEANER AND NICER -> is there ANY reason to use a table as also an array?
- increment ref counts when appending to a list or adding table entry 
- only increment ref count for gc obj upon assignment to a variable. that means, if we create a table, we don't auto set refcount to 1, and also must instantly delete if we don't assign to a variable at all.
- release GC reference if lose reference, and then propagate to element gc objects (both lists and maps)

### Language features

- reference counting TESTS
- fix cyclic references. maybe disallow them
- STRINGS
  - fix strings being reference counted objects...
- use custom assert for mesascript runtime
- access map elements via dot (e.g. map.x or map.f(param))

- need way to delete a list/table entry (remember to release ref count) https://docs.python.org/3/tutorial/datastructures.html#the-del-statement
  - deleting an entry is different from deleting the object stored in that entry. if we can delete entire objects, then we are able to destroy objects that are still referenced by other objects or variables...which would require tracking down every reference and removing them (otherwise they would be pointing to a "deleted" or "null" GCObject).  
- rename tables to MAP or DICTIONARY
- answer question about accessing local variables from other scopes

- add relops for GCObject type TValues. for other ops, just crash? 

- initialize table elements

- while loops & break
- for loops
- floats
- elifs

Way the fuck down the line:
- replace all std data structs with alternate optimized implementations in C?
- REFACTOR

### Bugs

- GUI incorrect zones when window resize
  - prob cuz mouse x y will be incorrect.
- Make sure vertext height thing didn't regress (text height inconsistency for loading vs forming vertices)

### Uncategorized

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




