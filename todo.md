# Quick thoughts

Hotline Miami x Nuclear Throne kind of game
- Not tile-baseddd
- Lots of particles -> blood sprites
- Do something interesting with the camera
- Dodge to avoid bullets -> faster than NT but slower than HM

# Known Bugs

- GUI fresh ID is not great...will be buggy when number of elements change dynamically
  - should change to be managed by the window (elements in same window will persist the same ids).
- code editor upon input, ensure cursor is visible by updating scroll values

# Immediate next

- math.lerp (different variations too) // exponential fall off of the speed
- pl: Arrays
- pl: % BinOp
- pl: elif
- pl: +=, -=, /=, *=
- pl: for (n in [1, 2, 3, 5, 7, 11])
- pl: for (k,v in [ a: 2, b: 3 ])
- pl: 'continue' and 'break' in while and for-loops
- code editor: cut, copy, paste
- code editor: ctrl+backspace and ctrl+delete for word deletion
- code editor: ctrl+d for duplicate line
- code editor: insert mode + indicator (?)
- code editor: SCROLLING

# Todo

- save/load gamedata
- boot sound (SDL_mixer)
- Markdown reader - manual in markdown, toggled with F1
- code editor: pg up pg down (maybe for functions?)
- code editor: ctrl+f

## piplang general
 - math ops: flr, ceil, rnd -> all return integer, max, min
 - More string operations
 - Should we assert BOOLEAN type for JUMP_IF_FALSE?

### Uncategorized

- Sprite sheet & animations
- Sprite batch rendering

- camera API (maybe not? user can implement own camera)
- gui API
- juice API
- sfx & music API
- juice API
- sfx & music API

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
- rework MesaIMGUI keyboard input handling. 

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

- Editor code coloring with text vertex colors
- fixed bug where primitive drawing was doing vb size * 0.2 instead of divide by 6...
- code editor scrolling
- piplang VM EPIC
- Fixed bug with "return 'string' or return MakeZoo()" where the transient object gets destroyed before being made transient at one scope higher.
- code editor: draw selection highlighting
- code editor: mouse dragging for selection
- code editor: mouse clicking to move cursor
- code editor: tab to spaces
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
