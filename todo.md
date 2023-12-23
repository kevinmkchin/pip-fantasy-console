# Quick thoughts

maybe ; starts a line comment? like Lisp and DrRacket. # could also start line comment. What about block comment?
rework MesaIMGUI keyboard input handling. 

# Known Bugs

- GUI fresh ID is not great...will be buggy when number of elements change dynamically
  - should change to be managed by the window (elements in same window will persist the same ids).

# Immediate next

- code editor: tab to spaces
- code editor: mouse interactions (clicking, dragging to select, is there dragging to move?)
- code editor: cut, copy, paste, and selection highlighting
- code editor: ctrl+backspace and ctrl+delete for word deletion
- code editor: insert mode + indicator (?)
- code editor: pg up pg down (maybe for functions?)

- gfx api: draw rect function
- piplang: When exception thrown, print warning and let program continue

# Todo

- boot sound (SDL_mixer)
- Markdown reader - manual in markdown, toggled with F1

### Language features
- Script modulus %
- Script profiler


map = { }
- map initialization
- access map elements via dot (e.g. map.x or map.f(param))
- allow chaining list or map access like: list[4]["x"][2] (an access of an access of an access)


- need way to delete a list/table entry (remember to release ref count) https://docs.python.org/3/tutorial/datastructures.html#the-del-statement
  - deleting an entry is different from deleting the object stored in that entry. if we can delete entire objects, then we are able to destroy objects that are still referenced by other objects or variables...which would require tracking down every reference and removing them (otherwise they would be pointing to a "deleted" or "null" GCObject).


Libraries/Packages
- math ops: flr, ceil, rnd -> all return integer


- introducing "null" or "nil" means there are always going to have to be null checks. instead, provide a function to check if a variable name exists or is alive.
- you should be able to ask for a list of identifiers/names that reference a given GCObj -> if a GCObj has 4 refs, I should be able to find out what those refs are...although that might be tough if the reference has no identifier e.g. if the reference is from inside a list or map entry.


Low priority Items:
- use custom assert for mesascript runtime
- rename tables to MAP or DICTIONARY
- while loops & break
- for loops
- elifs
- +=, -=, /=, *=*
- Better string support: concat strings and values, less friction like JavaScript, stuff like string + number operations
- reference counting TESTS
- add relops for GCObject type TValues. for other ops, just crash? 
- maybe we don't need an integer type (everything can be real?)
- Do something about cyclic references (maybe a tracing garbage collector just for cyclic refs)
- Let current scope access every scope up until the last function scope (for loop scope can access fn scope)

### Uncategorized

- Sprite sheet & animations
- Sprite batch rendering

- Editor camera & game camera API
- Game GUI API
- Game file management
  - meta data (Title, Cover art, Author, etc.)
  - everything needed to play or edit the game
- juice API
- Sound effects and music API

- Collision and dispatch system: Pixel perfect AABB Collision checks -> GJK&EPA
  - Select collider type from AABB, Sphere, and Convex point cloud


# Way the fuck down the line
### Uncategorized
- lots of potential optimizations in MesaIMGUI
- MesaIMGUI abstract out behaviour code so reusable (e.g. button behaviour)
- MesaIMGUI should probably eat keyboard inputs
- MesaIMGUI overlapping elements or buttons
- SDL_SCANCODE independent input key enums (or maybe unnecessary...)
- (fuck off until later) vertext rewrite for better usability better API

### Language features
- replace all std data structs with alternate optimized implementations in C?
- REFACTOR
- Better unit test suite (built into the language would be nice, it should be dog simple to use)
- should we have protected fields? how would this lead to encapsulation and inheritance?

### Branch off for rearchitecture

Core (essentially the common library that everything uses)
- MesaMath
- MesaCommon
- MesaUtility
- MemoryAllocator
- FileSystem
- PrintLog
- Console?

- Parsers
- Profiler
- Config

Gfx (probably don't make a module out of this)
- Renderer
- Shader
- DataTypesAndUtility

# Done

- run active/current script
- single line console quick save load text files
- replace code editor backend with stb_textedit.h (and some bugfixes from implementing the API)
- send SDL keyboard inputs directly to editor / code editor for processing.
- MesaScript change formal program defintion to: program : (procedure_decl | statement) (program)* Basically allow statements at script top-level. Added top-level script execution.
* place instances of entity assets in world (sorta)
* create new entity assets and set sprite and code (sorta)
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
