

fn dosmth(table)
{
    table[69] = v
    ~ should this fn be able to access v? probably not unless it's global?
    ~ if this fn tries to declare a new local variable v, it'll overwrite v from test
}

fn test()
{
  str = "bruh"

  a = {}
  b = a

  a[str] = 77
  a["yolo"] = 33

  v = 32

  ~ b[0] = 16 + v
  ~ a[1] = {}

  ~ b = a[1]
  ~ b[1] = 4

  ~ dosmth(a)

  printv(str)
  printv(a["bruh"])
  printv(a["yolo"])

  ~printv(b == a)
  ~printv(b.x == a.x)
}

test()

`
fn fib (n) 
{
  if (n < 2) 
  {
    return n
  } 
  else 
  {
    return fib(n - 1) + fib(n - 2)
  }
}

printv(fib(10))
~printv(fib(4))
printv(add(add(4, 2), 8))
`

`
~ the user could create levels in sprites (one sprite per layer) where color determines tile

fn mesa_init ()
{
    // setup collision relationships
    // player can collide with enemies
    // player can collide with level
    // enemies can collide with level
}

fn mesa_tick ()
{
    // update game obj 0

    // update player

    // update enemies
}

fn mesa_draw()
{
    // draw level
    // draw player
    // draw enemies
}
`
