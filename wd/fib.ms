

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

}
`

printv(32)
