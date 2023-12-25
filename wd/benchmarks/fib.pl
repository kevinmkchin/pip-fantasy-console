
fncalls = 0

fn fib(n)
{
  fncalls = fncalls + 1

  if (n < 2)
  {
    return n
  }
  else
  {
    return fib(n - 1) + fib(n - 2)
  }
}

fib(20)

`
  even just removing fncalls = fncalls + 1 almost halves the execution time because fncalls is at base of scopes.

Steps

  fncalls = fncalls + 1
    InterpretExpression(fncalls + 1)
    AST VARIABLE fncalls
      KeyExistsInCurrentScope O(1) (string hash x 1 or 2)
      AccessAtKey O(numScopes)
        for every scope up to right scope:
          Contains (string hash x 1)
        AccessMapEntry (string hash x 1)

    KeyExistsInCurrentScope O(1) (string hash x 1 or 2)
    ReplaceAtKey O(numScopes)
      for every scope up to right scope:
        Contains (string hash x 1)
      ReplaceMapEntryAtKey (string hash x 2)

  if (n < 2)
    KeyExistsInCurrentScope O(1) (string hash x 1 or 2)
    AccessAtKey (here its O(1) because n is in the latest scope)
      Contains (string hash x 1)
      AccessMapEntry (string hash x 1)

  return fib(n - 1) + fib(n - 2)
    getting n is O(1) albeit with unnecessary hashes
    fib is defined at the top level (script wide) so need to iterate all n scopes before finding fib
      activeEnv.KeyExists O(n)
      activeEnv.AccessAtKey O(n) again
    
`
