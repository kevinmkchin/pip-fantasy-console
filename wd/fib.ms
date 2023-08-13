`
LIST:
access element with a non negative integer like so:
    fruits[0]
    fruits[2]

fruits = {'orange', 'apple', 'pear', 'banana', 'kiwi', 'apple', 'banana'}
count(fruits, 'apple')
    > gives 2
index(fruits, 'apple')
    > gives 1

fruits.reverse()
fruits.append
remove
insert

MAP:
access element with string like so:
    map["the key"]
    map.anotherkey
    map.keyToFunction();

map = {'jack': 4098, 'sape': {feafe} }

d = {'k1': 1, 'k2': 2, 'k3': 3}

del d['k2']
print(d)
# {'k1': 1, 'k3': 3}

`

fn dosmth(table)
{
    table[69] = v
    ~ should this fn be able to access v? probably not unless it's global?
    ~ if this fn tries to declare a new local variable v, it'll overwrite v from test
}


`
fn passByRef(loi)
{
    loi[0] = loi[0] * 11 
}

fn caller()
{
    l = [ 7 ]
    passByRef(l)
    print(l)
}

caller()
`


fn GetPlayer()
{
    player = {}
    player["x"] = 32
    return player
}

fn PrintPlayer()
{
    _player = GetPlayer()
    print(_player["x"])
}

~PrintPlayer()


`
fn test()
{
  str = "bruh"

  a = {}
  b = a

  v = 32

  li = [ 0, 49, "50", 12390291, raise_to(2,3) ]

  a[str] = 77
  a["yolo"] = 33

  li[0] = add(16 ,v)
  li[1] = {}
  li[3] = 1000

  ~ b = a[1]
  ~ b[1] = 4

  ~ dosmth(a)

  print("printing string and table here:")
  print(str)
  print(a[str])
  print(a["bruh"])
  print(a["yolo"])
  print("------")
  print("printing list here:")
  print(li)

  ~printv(b == a)
  ~printv(b.x == a.x)
}
`
~test()



~Find the n-th Fibonacci number
fn fib (n) 
{
  if (n < 2) {
    return n
  } else {
    return fib(n - 1) + fib(n - 2)
  }
}
checkeq(55, fib(10))
checkeq(3, fib(4))
checkeq(14, add(add(4, 2), 8))

~Find the factorial of n
fn fact(n)
{
    if (n == 0) { return 1 }
    else { return n * fact(n-1) }
}
checkeq(1, fact(0))
checkeq(1, fact(1))
checkeq(24, fact(4))
checkeq(362880, fact(9))

fn StringIsPassedByValueTest()
{
    str = "string is passed by value test"
    someList = [ str, str, str ]
    someMap = {}
    someMap["someIndex"] = str
    ~at this point, there are 5 strings, and str still only has one refcount

    checkeq(refcount(str), 0) ~refcount returns 0 because it checks the refcount of a str copy
}
StringIsPassedByValueTest();

fn Helper_RefCountTests_DeeperScope(obj)
{
    checkeq(refcount(obj), 5);
    a = obj;
    b = obj;
    c = obj;
    checkeq(refcount(obj), 8);
}

fn RefCountTests()
{
    someList = [ 1, 2, "hello", 32]
    someListRef1 = someList
    someListRef2 = someList
    someListRef3 = someList

    checkeq(refcount(someList), 4)
    Helper_RefCountTests_DeeperScope(someList)
    checkeq(refcount(someList), 4)
}
RefCountTests()


` BRAINSTORMING INTEGRATION:
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
