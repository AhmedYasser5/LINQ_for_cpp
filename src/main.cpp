#include "Pipeline.hpp"
#include <initializer_list>
#include <iostream>
#include <vector>
using namespace std;
using Pipeline::Composer;
using Pipeline::OrderBy;
using Pipeline::Select;
using Pipeline::Take;
using Pipeline::Where;

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
  auto out = Composer<int>()
                 .Select(addOne)
                 .Select(square)
                 .Select(subtract10)
                 .Where(greater5)
                 .Take(5)
                 .OrderBy(comparer)
                 .Take(2)
                 .OrderBy(less<int>())
                 .ToList(in);
  print(out);
  return 0;
}
