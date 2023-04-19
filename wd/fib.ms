

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
printv(fib(4))
printv(add(232, 250))

