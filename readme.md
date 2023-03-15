# ARCADIA Fantasy Console
### By Kevin Myungkeon Chin

- Easy to learn
- Frictionless
- Limitations to scope ambition (USEFUL CONSTRAINTS - "useful" for gamedev)
- Engine is only as good as its tools, documentation, and error messages
- For Mac, Windows, Linux

## Todo

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


## MVP
- Sprites and import pngs (objects?)
- Game logic scripting engine

## Add after MVP
- Screen system and screen switching
- Collision and dispatch system
- Sound effects and music system




## Build on Mac

### Command line
```
cmake -S <cmakelists.txt source directory> -B <output directory>
cmake --build <output directory>
```
### CLion
Open CMakeLists.txt in root with CLion