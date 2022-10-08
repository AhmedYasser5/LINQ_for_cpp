#include "Pipeline.hpp"
#include <iostream>
#include <vector>
using namespace std;

template <typename T> void print(const vector<T> &vec) {
  for (auto &it : vec)
    cout << it << ' ';
  cout << endl;
}
int main() {
  auto addOne = [](const int &x) -> int {
    cout << "I am adding 1 to " << x << endl;
    return x + 1;
  };
  auto square = [](const int &x) -> int {
    cout << "I am squaring " << x << endl;
    return x * x;
  };
  auto greater5 = [](const int &x) -> bool {
    bool pass = x > 5;
    cout << "I am " << (pass ? "" : "not ") << "passing " << x << endl;
    return pass;
  };
  auto subtract10 = [](const int &x) -> int {
    cout << "I am subtracting 10 from " << x << endl;
    return x - 10;
  };
  auto comparer = [](const int &x, const int &y) -> bool {
    cout << "I am comparing " << x << " with " << y << endl;
    return x > y;
  };
  auto in = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  Pipeline::Composer<int> com;
  auto out = com.Select(addOne)
                 .Select(square)
                 .Select(subtract10)
                 .Where(greater5)
                 .Take(5)
                 .OrderBy(comparer)
                 .Take(2)
                 .OrderBy(less<int>())
                 .ToList(in);
  print(out);
  com.clear();
  com.Select(addOne).Select(addOne).Select(addOne);
  Pipeline::Composer<int> com1 = com, com2 = com;
  com1.Select(addOne).Select(addOne);
  com2.Select(subtract10);
  cout << com.ToList({1}).front() << endl;
  cout << com1.ToList({1}).front() << endl;
  cout << com2.ToList({1}).front() << endl;
  com1.append(com);
  com.append(com);
  Pipeline::Composer<int> com3 = com;
  cout << com3.ToList({1}).front() << endl;
  return 0;
}
