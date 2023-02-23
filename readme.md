# ARCADIA Fantasy Console
### By Kevin Myungkeon Chin

- Easy to learn
- Frictionless
- Limitations to scope ambition
- Engine is only as good as its tools, documentation, and error messages


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
