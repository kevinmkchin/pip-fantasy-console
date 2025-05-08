# Pip Fantasy Console (work in progress)

Pip Fantasy Console is a work-in-progress game making tool inspired by fantasy consoles like PICO-8. Although unfinished, I coded some really cool stuff for this that is worth sharing.

![](https://kevin.gd/images/pip-demo/mario.png)
> Sprite editor

Pip provides a sprite editor to create art assets and a scripting engine to execute gameplay code. The core systems to create a basic game are there.

![](https://kevin.gd/images/pip-demo/nuclear-pip.gif)
> Top-down game prototype made in Pip

The user can create sprites in the sprite editor. Undo-redo is implemented via byte serialization. The editor UI is created using a custom GUI library written from scratch.

![](https://kevin.gd/images/pip-demo/timelapse.gif)
> Sprite/pixel art editor in action

The user can enter the script editor to code the gameplay. I created a custom scripting language for Pip.

![](https://kevin.gd/images/pip-demo/piplang-demo-1.png)
> Example script

Pip's scripting language is a typeless language designed to be simple to learn and fun to mess around with. It follows the core ethos of this fantasy console of being "playful". It supports most common language features as well as heap objects like dynamic arrays, strings, and hashmaps. 

![](https://kevin.gd/images/pip-demo/piplang-demo-2.png)
> Objects and functions

With some simple scripting, we can recreate Pong.

![](https://kevin.gd/images/pip-demo/pong.gif)
> Pong recreated


Script for pong game below:
```

mut ballspeed = 175
mut paddlespeed = 2

mut ball = { 
  "x":-5, 
  "y":-5, 
  "w":10, 
  "h":10,
  "xvel": ballspeed,
  "yvel": 0
}

mut player1 = {
  "x": -110,
  "y": -25,
  "w": 10,
  "h": 50,
  "score": 0
}

mut player2 = {
  "x": 90,
  "y": -25,
  "w": 10,
  "h": 50,
  "score": 0
}

fn iscolliding(rect1, rect2)
{
  mut colliding = 
    rect1.x < rect2.x + rect2.w and
    rect1.x + rect1.w > rect2.x and
    rect1.y < rect2.y + rect2.h and
    rect1.y + rect1.h > rect2.y
  return (colliding)
}

fn calculateballdirection(collidedplayer)
{
  mut ballcentery = ball.y + ball.h / 2
  mut playercentery = collidedplayer.y + collidedplayer.h / 2
  mut yintersect = ballcentery - playercentery
  mut normalizedyintersect = yintersect / (collidedplayer.h / 2)
  if (normalizedyintersect > 1)
    normalizedyintersect = 1
  if (normalizedyintersect < -1)
    normalizedyintersect = -1
  mut bounceangle = normalizedyintersect * (60/360)*(3.14159265*2)

  if (ball.xvel < 0)
    ball.xvel = ballspeed * math.cos(bounceangle)
  else
    ball.xvel = -ballspeed * math.cos(bounceangle)
  ball.yvel = ballspeed * math.sin(bounceangle)
}

fn moveplayers()
{
  if (ctrl.up)
    player2.y = player2.y - paddlespeed
  if (ctrl.down)
    player2.y = player2.y + paddlespeed

  if (ctrl.w)
    player1.y = player1.y - paddlespeed
  if (ctrl.s)
    player1.y = player1.y + paddlespeed
}

fn moveball()
{
  ball.x = ball.x + ball.xvel * time.dt
  ball.y = ball.y + ball.yvel * time.dt

  if (ball.y > 120)
    ball.yvel = -ball.yvel
  if (ball.y < -120)
    ball.yvel = -ball.yvel

  if (ball.x > 140)
  {
    ball.x = -5
    ball.y = -5
    player1.score = player1.score + 1
    print("player 1 score")
    print(player1.score)
    print("player 2 score")
    print(player2.score)
    print("  ")
  }
  if (ball.x < -140)
  {
    ball.x = -5
    ball.y = -5
    player2.score = player2.score + 1
    print("player 1 score")
    print(player1.score)
    print("player 2 score")
    print(player2.score)
    print("  ")
  }

  if (ball.xvel < 0 and iscolliding(ball, player1))
    calculateballdirection(player1)
  if (ball.xvel > 0 and iscolliding(ball, player2))
    calculateballdirection(player2)
}

gfx.camx = -160
gfx.camy = -120

fn tick()
{
  moveplayers()
  moveball()

  gfx.clear({ "r":0, "g":0, "b":0 })
  gfx.drawrect(ball, { "r":255, "g":255, "b":255 })
  gfx.drawrect(player1, { "r":255, "g":255, "b":255 })
  gfx.drawrect(player2, { "r":255, "g":255, "b":255 })
}


```


## Build on Windows

```
build
build clean - clean build directory
build release - release build 

run
run release
run vs - start vs sln
run subl - start sublime project
```

### Visual Studio
Open repo directory with Visual Studio (requires Cmake tools for VS to be installed).
Right click on CMakeLists.txt from the solution explorerer and "Set as Startup Item".

### CLion
Open CMakeLists.txt in root with CLion

## Build on Mac

### Command line
```
cmake -S <cmakelists.txt source directory> -B <output directory>
cmake --build <output directory>
```
### CLion
Open CMakeLists.txt in root with CLion
