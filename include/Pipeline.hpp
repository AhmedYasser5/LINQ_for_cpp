#include <algorithm>
#include <deque>
#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace Pipeline {

  using std::deque;
  using std::function;
  using std::initializer_list;
  using std::make_shared;
  using std::make_unique;
  using std::move;
  using std::shared_ptr;
  using std::size_t;
  using std::unique_ptr;
  using std::vector;

  template <typename T> class Functor {
  public:
    virtual ~Functor() = default;
    virtual void
    setPreviousFunction(shared_ptr<Functor<T>> previousFunction) = 0;
    virtual shared_ptr<Functor<T>> getPreviousFunction() const = 0;
    virtual inline unique_ptr<T> operator()(const bool &reset) = 0;
  };

  template <typename T> class Select : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    function<T(const T &)> updater;

  public:
    Select(const function<T(const T &)> &updater)
        : previousFunction(nullptr), updater(updater) {}
    Select(const Select<T> &other) = default;
    Select(Select<T> &&other) = default;
    Select<T> &operator=(const Select<T> &other) = default;
    Select<T> &operator=(Select<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      auto result = previousFunction->operator()(reset);
      if (result != nullptr)
        result = make_unique<T>(updater(*result));
      return result;
    }
  };

  template <typename T> class Where : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    function<bool(const T &)> checker;

  public:
    Where(const function<bool(const T &)> &checker)
        : previousFunction(nullptr), checker(checker) {}
    Where(const Where<T> &other) = default;
    Where(Where<T> &&other) = default;
    Where<T> &operator=(const Where<T> &other) = default;
    Where<T> &operator=(Where<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      unique_ptr<T> result;
      bool needReset = reset;
      do {
        result = previousFunction->operator()(needReset);
        needReset = false;
      } while (result != nullptr && !checker(*result));
      return result;
    }
  };

  template <typename T> class Take : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    size_t remaining;
    const size_t capacity;

  public:
    Take(const size_t &capacity)
        : previousFunction(nullptr), remaining(capacity), capacity(capacity) {}
    Take(const Take<T> &other) = default;
    Take(Take<T> &&other) = default;
    Take<T> &operator=(const Take<T> &other) = default;
    Take<T> &operator=(Take<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      if (reset)
        remaining = capacity;
      if (!remaining)
        return nullptr;
      --remaining;
      auto result = previousFunction->operator()(reset);
      if (result == nullptr)
        remaining = 0;
      return result;
    }
  };

  template <typename T> class OrderBy : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    function<bool(const T &, const T &)> comparer;
    bool processed;
    deque<unique_ptr<T>> results;

  public:
    OrderBy(const function<bool(const T &, const T &)> &comparer)
        : previousFunction(nullptr), comparer(comparer), processed(false) {}
    OrderBy(const OrderBy<T> &other) = default;
    OrderBy(OrderBy<T> &&other) = default;
    OrderBy<T> &operator=(const OrderBy<T> &other) = default;
    OrderBy<T> &operator=(OrderBy<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      if (reset) {
        processed = false;
        results.clear();
      }
      if (!processed) {
        bool needReset = reset;
        unique_ptr<T> result;
        while (true) {
          result = previousFunction->operator()(needReset);
          if (result == nullptr)
            break;
          results.emplace_back(move(result));
          needReset = false;
        }
        sort(results.begin(), results.end(),
             [this](const unique_ptr<T> &first, const unique_ptr<T> &second)
                 -> bool { return comparer(*first, *second); });
      }
      if (results.empty())
        return nullptr;
      unique_ptr<T> result = nullptr;
      result.swap(results.front());
      results.pop_front();
      return result;
    }
  };

  template <typename T> class Composer : public Functor<T> {
    shared_ptr<Functor<T>> first, last;

    template <typename C> inline vector<T> preprocess(const C &values) {
      auto base = last->getPreviousFunction();
      last->setPreviousFunction(make_shared<Iterate>(values));
      auto result = processAll();
      last->setPreviousFunction(base);
      return result;
    }
    inline vector<T> processAll() {
      vector<T> results;
      bool reset = true;
      while (true) {
        auto result = first->operator()(reset);
        if (result == nullptr)
          break;
        results.emplace_back(move(*result));
        reset = false;
      }
      return results;
    }

    class Iterate : public Functor<T> {
      typename vector<T>::const_iterator it, end;

    public:
      template <typename C>
      Iterate(const C &values) : it(values.begin()), end(values.end()) {}
      Iterate(const Iterate &other) = default;
      Iterate(Iterate &&other) = default;
      Iterate &operator=(const Iterate &other) = default;
      Iterate &operator=(Iterate &&other) = default;
      void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {}
      shared_ptr<Functor<T>> getPreviousFunction() const { return nullptr; }
      inline unique_ptr<T> operator()(const bool &reset) {
        if (it == end)
          return nullptr;
        auto result = make_unique<T>(*it);
        ++it;
        return result;
      }
    };

  public:
    Composer() : first(nullptr), last(nullptr) {}
    Composer(const Composer<T> &other) = default;
    Composer(Composer<T> &&other) = default;
    Composer<T> &operator=(const Composer<T> &other) = default;
    Composer<T> &operator=(Composer<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      last->setPreviousFunction(previousFunction);
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return last->getPreviousFunction();
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      return first->operator()(reset);
    }
    template <typename F> Composer<T> &append(F func) {
      func.setPreviousFunction(first);
      first = make_shared<F>(move(func));
      if (last == nullptr)
        last = first;
      return *this;
    }
    template <typename... Args> Composer<T> &Select(Args &&...args) {
      return append(Pipeline::Select<T>(args...));
    }
    template <typename... Args> Composer<T> &Take(Args &&...args) {
      return append(Pipeline::Take<T>(args...));
    }
    template <typename... Args> Composer<T> &OrderBy(Args &&...args) {
      return append(Pipeline::OrderBy<T>(args...));
    }
    template <typename... Args> Composer<T> &Where(Args &&...args) {
      return append(Pipeline::Where<T>(args...));
    }
    inline vector<T> ToList(const initializer_list<T> &values) {
      return preprocess(values);
    }
    template <typename C> inline vector<T> ToList(const C &values) {
      return preprocess(values);
    }
  };

} // namespace Pipeline
