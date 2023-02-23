# ARCADIA Fantasy Console
### By Kevin Myungkeon Chin

- Easy to learn
- Frictionless
- Limitations to scope ambition
- Engine is only as good as its tools, documentation, and error messages

## Todo

- Editor GUI
- Game GUI atomics
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




## Build on Mac

### Command line
```
cmake -S <cmakelists.txt source directory> -B <output directory>
cmake --build <output directory>
```

### Xcode
```
cmake -S <cmakelists.txt source directory> -B <output directory> -G Xcode
sudo chown -R <macos username> <output directory>
sudo chmod -R 774 <output directory>
```
Then open Xcode project and run Game project.

### Troubleshooting
If `-- The CXX compiler identification is unknown` when trying to generate project:
```
sudo xcode-select --reset
```
