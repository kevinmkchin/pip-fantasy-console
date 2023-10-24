# Known Bugs

- GUI fresh ID is not great...will be buggy when number of elements change dynamically
  - should change to be managed by the window (elements in same window will persist the same ids).


# Next
- some refactor
- clicky into world viewer
    - with zoom
    - higher resolution render? maybe?

* place instances of entity assets in world
* create new entity assets and set sprite and code
* entity code -> game behaviour pipeline is further refined (e.g. self? how to use local variables? global vars? how to refer to other ents?)

- move fib.ms test code into the editor itself. maybe



# Todo

- Markdown reader - manual in markdown, toggled with F1
- boot sound (SDL_mixer)
- Depth for GUI? depth ranges and depth test and shit?


MESASCRIPT EPIC:
- Complete MesaScript a0.1 -> MesaGUI code editor -> select entity type to edit their code -> entity type can choose sprite
- MesaScript integration
- MesaGUI code editor
  - better inputs (ctrl and shift modifier checks)
- profiler

### Language features

- allow chaining list or map access like: list[4]["x"][2] (an access of an access of an access)
- access map elements via dot (e.g. map.x or map.f(param))
- map initialization

- math ops: flr, ceil, rnd -> all return integer
- stuff like string + number operations

- while loops & break 
- for loops
- Let current scope access every scope up until the last function scope (for loop scope can access fn scope)

- need way to delete a list/table entry (remember to release ref count) https://docs.python.org/3/tutorial/datastructures.html#the-del-statement
  - deleting an entry is different from deleting the object stored in that entry. if we can delete entire objects, then we are able to destroy objects that are still referenced by other objects or variables...which would require tracking down every reference and removing them (otherwise they would be pointing to a "deleted" or "null" GCObject).
- rename tables to MAP or DICTIONARY

- elifs
- +=, -=, /=, *=*

- Do something about cyclic references (maybe a tracing garbage collector just for cyclic refs)
- Better string support: concat strings and values, less friction like JavaScript
- reference counting TESTS
- use custom assert for mesascript runtime
- add relops for GCObject type TValues. for other ops, just crash? 
- maybe we don't need an integer type (everything can be real?)


- introducing "null" or "nil" means there are always going to have to be null checks. instead, provide a function to check if a variable name exists or is alive.
- you should be able to ask for a list of identifiers/names that reference a given GCObj -> if a GCObj has 4 refs, I should be able to find out what those refs are...although that might be tough if the reference has no identifier e.g. if the reference is from inside a list or map entry.

### Uncategorized

- Game GUI atomics
- Editor camera & game camera
- Collision and dispatch system: Pixel perfect AABB Collision checks -> GJK&EPA
  - Select collider type from AABB, Sphere, and Convex point cloud
- Sound effects and music system
- Sprite sheet & animations
- Sprite batch rendering
- Game file management
  - meta data (Title, Cover art, Author, etc.)
  - everything needed to play or edit the game
- Space system
- Entity system
- Console
- Juice and game specific utilities stuff


### Way the fuck down the line
##### Uncategorized
- lots of potential optimizations in MesaIMGUI
- MesaIMGUI abstract out behaviour code so reusable (e.g. button behaviour)
- MesaIMGUI should probably eat keyboard inputs
- MesaIMGUI overlapping elements or buttons
- SDL_SCANCODE independent input key enums
- (fuck off until later) vertext rewrite for better usability better API

##### Language features
- replace all std data structs with alternate optimized implementations in C?
- REFACTOR
- Better unit test suite (built into the language would be nice, it should be dog simple to use)
- should we have protected fields? how would this lead to encapsulation and inheritance?


# Done

- Import temporary placeholder sprites
- Basic GUI masking
- Basic GUI retained mode auto layouting
- DesktopWindowsManager DPI Awareness to System, and SDL_HINT_WINDOWS_DPI_SCALING to 0. Stuff isn't blurry anymore thank fuck.
- DesktopWindowsManager Flush to improve opengl vsync stutter on Windows devices while program is windowed
- FLOATS
- some major parser bugs:
  - parser bug: checking parentheses in an expression should only happen at smallest factor level
  - parser bug: logical negation should just recall cond_equal but it was calling cond_or or factor
  - some mistakes in BINOP and RELOP interpreter that was using int value instead of real value
- disallow accessing local function variables from other function scopes
- disallow map access by index (int)
- allow list construction like so [[1, 2, 3], [4, 5], [6, 7, 8, 9]] by being transient upon construction
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
