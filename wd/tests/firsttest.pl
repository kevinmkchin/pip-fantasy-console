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
    A = [ [1, 2, 3], 4, [5, 6] ]
    B = A[2]
    checkeq(refcount(B), 2)

    someList = [ 1, 2, "hello", 32]
    someListRef1 = someList
    someListRef2 = someList
    someListRef3 = someList

    checkeq(refcount(someList), 4)
    Helper_RefCountTests_DeeperScope(someList)
    checkeq(refcount(someList), 4)
}
RefCountTests()

checkeq(1.0, true)
checkeq(7, 7.0)
checkeq(add(3.14, 2.77), 5.91)
checkeq(2.71 * 9.5, 25.745)
checkeq(2.71 - 9.5, -6.79)
checkeq(9 + 2.71, 11.71)
checkeq(2.71/2.71, 1.0)
checkeq(100000.0/100000, 1)
checkeq((2.71/9.5) - 0.285263 < 0.0001 and (2.71/9.5) - 0.285263 > -0.0001, true)

