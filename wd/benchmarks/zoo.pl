fn MakeAardvark() {
  aardvark = {}
  aardvark['name'] = "jeff"
  aardvark['food'] = "apple"
  aardvark['age'] = 10
  return aardvark
}

fn MakeAnimal(name, food, age)
{
  animal = {}
  animal['name'] = name
  animal['food'] = food
  animal['age'] = age
  return animal
}

fn MakeZoo()
{
  zoo = {}
  zoo['aardvark'] = MakeAardvark()
  zoo['barracuda'] = MakeAnimal("barry", "sardines", 4)
  zoo['caribou'] = MakeAnimal("cassie", "pineapple", 20)
  zoo['donkey'] = MakeAnimal("dan", "carrot", 12)
  zoo['elephant'] = MakeAnimal("dumbo", "watermelon", 28)
  zoo['fox'] = MakeAnimal("Mr Fox", "cider", 16)
  return zoo
}

fn MakeZooAfterNScopes(n)
{
  if (n == 1)
  {
    return MakeZoo();
  }
  else
  {
    return MakeZooAfterNScopes(n - 1)
  }
}

fn GetCombinedAgeAfterNScopes(n, z)
{
  if (n == 1)
  {
    a = z['aardvark']
    b = z['barracuda']
    c = z['caribou']
    d = z['donkey']
    e = z['elephant']
    f = z['fox']
    return a['age'] + b['age'] + c['age'] + d['age'] + e['age'] + f['age']
  }
  else
  {
    return GetCombinedAgeAfterNScopes(n - 1, z);
  }
}

accumulateAge = 0;
i = 0
n = 10000
while (i < n){
  z = MakeZooAfterNScopes(5)
  combinedAge = GetCombinedAgeAfterNScopes(2, z);
  accumulateAge = accumulateAge + combinedAge
  i = i + 1
}

checkeq(accumulateAge, 90 * n)
