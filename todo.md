# TODO

- boot sound (SDL_mixer)
- vertext rewrite for better usability better API
- move fib.ms test code into the editor itself.


### Bugs

- GUI incorrect zones when window resize with black bars, window aspect ratio != game ratio

### Engine

MESASCRIPT EPIC:
- Complete MesaScript a0.1 -> MesaGUI code editor -> select entity type to edit their code -> entity type can choose sprite
- MesaScript integration
- MesaGUI code editor
  - better inputs (ctrl and shift modifier checks)

### Language features

- allow list construction like so [[1, 2, 3], [4, 5], [6, 7, 8, 9]] by being transient upon construction
- fix cyclic references. maybe disallow them
- FLOATS
  - flr, ceil, rnd
- access map elements via dot (e.g. map.x or map.f(param))
- need way to delete a list/table entry (remember to release ref count) https://docs.python.org/3/tutorial/datastructures.html#the-del-statement
  - deleting an entry is different from deleting the object stored in that entry. if we can delete entire objects, then we are able to destroy objects that are still referenced by other objects or variables...which would require tracking down every reference and removing them (otherwise they would be pointing to a "deleted" or "null" GCObject).
- answer question about accessing local variables from other scopes
- rename tables to MAP or DICTIONARY
- while loops & break
- for loops
- elifs
- +=, -=, /=, *=*


- Better string support: concat strings and values, less friction like JavaScript
- reference counting TESTS
- use custom assert for mesascript runtime
- add relops for GCObject type TValues. for other ops, just crash? 
- initialize table elements


- introducing "null" or "nil" means there are always going to have to be null checks. instead, provide a function to check if a variable name exists or is alive.
- you should be able to ask for a list of identifiers/names that reference a given GCObj -> if a GCObj has 4 refs, I should be able to find out what those refs are...although that might be tough if the reference has no identifier e.g. if the reference is from inside a list or map entry.

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
- Space system
- Entity system
- Console



### Way the fuck down the line
##### Uncategorized
- lots of potential optimizations in MesaIMGUI
- MesaIMGUI should probably eat keyboard inputs
- MesaIMGUI overlapping elements or buttons
- SDL_SCANCODE independent input key enums

##### Language features
- replace all std data structs with alternate optimized implementations in C?
- REFACTOR
- Better unit test suite (built into the language would be nice, it should be dog simple to use)
- should we have protected fields? how would this lead to encapsulation and inheritance?


# Done

- STRINGS: strings should not be reference counted objects
  - strings upon construction are transient objects: if they aren't captured by a variable they will be deleted
  - strings are never referenced, always copied: if an expression evaluates to a string, InterpretExpression will always create and return a transient copy of it. thus it is impossible for a string's refCount to be more than 1. If it is 0, then it is transient and will be collected at the end of the statement.
- Better Transient objects tracking (one tracker per scope; cleared at the end of each statement at that scope)
  - keep track of transient GC objects at each scope depth and clear corresponding list after each statement at that depth
- fix return values being reference counted objects created locally (function scope about to be destroyed). function scope gets destroyed before return value is captured and assigned to a variable.
  - the following should work in the future: newstring = GetString() + " world"; newlist = GetList() + [1, b, 3];
- bugfix: GUI incorrect zones when window resize prob cuz mouse x y will be incorrect.
- index tables with strings
- initialize array elements
- sort of lists/arrays - actual array implementation <- MAYBE JUST MAKE AN ENTIRE NEW ARRAY TYPE??? THAT MIGHT BE SO MUCH CLEANER AND NICER -> is there ANY reason to use a table as also an array?
- increment ref counts when appending to a list or adding table entry 
- only increment ref count for gc obj upon assignment to a variable. that means, if we create a table, we don't auto set refcount to 1, and also must instantly delete if we don't assign to a variable at all.
- release GC reference if lose reference, and then propagate to element gc objects (both lists and maps)
- release ref of local variables when function scope gets popped
