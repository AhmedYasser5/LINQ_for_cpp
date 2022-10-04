#include "Pipeline.hpp"
#include <initializer_list>
#include <iostream>
#include <vector>
using namespace std;
using Pipeline::Compose;
using Pipeline::OrderBy;
using Pipeline::Select;
using Pipeline::Take;
using Pipeline::Where;

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
  auto composed = Pipeline::Compose<int>(
      Pipeline::Select<int>(addOne), Pipeline::Select<int>(square),
      Pipeline::Select<int>(subtract10), Pipeline::Where<int>(greater5),
      Pipeline::Take<int>(5), Pipeline::OrderBy<int>(comparer),
      Pipeline::Take<int>(2), Pipeline::OrderBy<int>(less<int>()));
  auto vec = composed({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  for (auto &it : vec)
    cout << it << ' ';
  cout << endl;
  return 0;
}
