#if DEBUG == 1
#include "pch.hpp"
#else
#include <bits/stdc++.h>
#endif
#include "Pipeline.hpp"
using namespace std;
#define endl '\n'
#define all(x) x.begin(), x.end()
#define sz(v) v.size()
#define nodebug 0
#if DEBUG == 1 && nodebug == 0
ofstream out("debug.txt");
#define debug(x) out << #x << "=" << x << '\n' << flush
#else
#define debug(x)
#endif
typedef long long ll;
const int dx[] = {-1, 0, 1, 0}, dy[] = {0, -1, 0, 1};
const int N = 9e6 + 10, INF = 1e8, MOD = 1e9 + 7;

int main() {
  ios::sync_with_stdio(0);
  cin.tie(0);
  cout.tie(0);
#if DEBUG == 1
  freopen("in.txt", "r", stdin);
  // freopen("out.txt", "w", stdout);
#endif
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
