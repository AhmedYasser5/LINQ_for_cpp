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
  using std::remove_reference;
  using std::shared_ptr;
  using std::unique_ptr;
  using std::vector;

  template <typename T> class Functor {
  public:
    virtual ~Functor() {}
    virtual inline unique_ptr<T> operator()(const bool &reset) = 0;
    virtual void
    setPreviousFunction(shared_ptr<Functor<T>> previousFunction) = 0;
  };

  template <typename T> class Select : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    function<T(const T &)> updater;

  public:
    Select(const function<T(const T &)> &updater)
        : previousFunction(nullptr), updater(updater) {}
    Select(Select<T> &&other) = default;
    ~Select() = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
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
    Where(Where<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
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
    Take(Take<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
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
    deque<T> results;

  public:
    OrderBy(const function<bool(const T &, const T &)> &comparer)
        : previousFunction(nullptr), comparer(comparer), processed(false) {}
    OrderBy(OrderBy<T> &&other) = default;
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
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
          results.emplace_back(*result);
          needReset = false;
        }
        sort(results.begin(), results.end(), comparer);
      }
      if (results.empty())
        return nullptr;
      auto result = make_unique<T>(results.front());
      results.pop_front();
      return result;
    }
  };

  template <typename T> class Compose {
    shared_ptr<Functor<T>> first, last;

    template <typename F> shared_ptr<Functor<T>> compose(F &&finalStep) {
      using FPure = typename remove_reference<F>::type;
      first = make_unique<FPure>(move(finalStep));
      return first;
    }
    template <typename F, typename... ARGS>
    shared_ptr<Functor<T>> compose(F &&step, ARGS &&...others) {
      using FPure = typename remove_reference<F>::type;
      shared_ptr<Functor<T>> result = make_shared<FPure>(move(step));
      compose(move(others)...)->setPreviousFunction(result);
      return result;
    }
    inline vector<T> processAll() {
      vector<T> results;
      bool reset = true;
      while (true) {
        auto result = first->operator()(reset);
        if (result == nullptr)
          break;
        results.emplace_back(*result);
        reset = false;
      }
      last->setPreviousFunction(nullptr);
      return results;
    }

    class IterateFunc : public Functor<T> {
      typename vector<T>::const_iterator it, end;

    public:
      template <typename C>
      IterateFunc(const C &values) : it(values.begin()), end(values.end()) {}
      void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {}
      inline unique_ptr<T> operator()(const bool &reset) {
        if (it == end)
          return nullptr;
        auto result = make_unique<T>(*it);
        ++it;
        return result;
      }
    };

  public:
    template <typename... ARGS> Compose(ARGS &&...args) {
      last = compose(args...);
    }
    inline vector<T> operator()(const initializer_list<T> &values) {
      last->setPreviousFunction(make_shared<IterateFunc>(values));
      return processAll();
    }
    template <typename C> inline vector<T> operator()(const C &values) {
      last->setPreviousFunction(make_shared<IterateFunc>(values));
      return processAll();
    }
    inline vector<T> ToList(const initializer_list<T> &values) {
      return operator()(values);
    }
    template <typename C> inline vector<T> ToList(const C &values) {
      return operator()(values);
    }
  };

} // namespace Pipeline
