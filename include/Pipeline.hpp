#pragma once

#include <algorithm>
#include <deque>
#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace Pipeline {

  using std::cbegin;
  using std::cend;
  using std::deque;
  using std::function;
  using std::initializer_list;
  using std::make_shared;
  using std::make_unique;
  using std::move;
  using std::shared_ptr;
  using std::size_t;
  using std::static_pointer_cast;
  using std::unique_ptr;
  using std::vector;

  template <typename T> class Select;
  template <typename T> class Take;
  template <typename T> class Where;
  template <typename T> class OrderBy;
  template <typename T> class Composer;
  template <typename T> class Iterate;

  template <typename T> class Functor {
    friend Select<T>;
    friend Take<T>;
    friend Where<T>;
    friend OrderBy<T>;
    friend Composer<T>;
    friend Iterate<T>;

  protected:
    virtual shared_ptr<Functor<T>> deepCopy() const = 0;
    virtual void
    setPreviousFunction(shared_ptr<Functor<T>> previousFunction) = 0;
    virtual shared_ptr<Functor<T>> getPreviousFunction() const = 0;
    virtual inline unique_ptr<T> operator()(const bool &reset) = 0;

  public:
    virtual ~Functor() = default;
  };

  template <typename T> class Select : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    function<T(const T &)> updater;

    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    shared_ptr<Functor<T>> deepCopy() const {
      auto copy = make_shared<Select<T>>(updater);
      if (previousFunction)
        copy->previousFunction = previousFunction->deepCopy();
      return copy;
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      auto result = previousFunction->operator()(reset);
      if (result != nullptr)
        result = make_unique<T>(updater(*result));
      return result;
    }

  public:
    Select(const function<T(const T &)> &updater)
        : previousFunction(nullptr), updater(updater) {}
    Select(const Select<T> &other)
        : previousFunction(other.previousFunction
                               ? other.previousFunction->deepCopy()
                               : nullptr),
          updater(other.updater) {}
    Select<T> &operator=(const Select<T> &other) {
      updater = other.updater;
      previousFunction =
          other.previousFunction ? other.previousFunction->deepCopy() : nullptr;
      return *this;
    }
    Select(Select<T> &&other) = default;
    Select<T> &operator=(Select<T> &&other) = default;
  };

  template <typename T> class Where : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    function<bool(const T &)> checker;

    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    shared_ptr<Functor<T>> deepCopy() const {
      auto copy = make_shared<Where<T>>(checker);
      if (previousFunction)
        copy->previousFunction = previousFunction->deepCopy();
      return copy;
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

  public:
    Where(const function<bool(const T &)> &checker)
        : previousFunction(nullptr), checker(checker) {}
    Where(const Where<T> &other)
        : previousFunction(other.previousFunction
                               ? other.previousFunction->deepCopy()
                               : nullptr),
          checker(other.checker) {}
    Where<T> &operator=(const Where<T> &other) {
      checker = other.checker;
      previousFunction =
          other.previousFunction ? other.previousFunction->deepCopy() : nullptr;
      return *this;
    }
    Where(Where<T> &&other) = default;
    Where<T> &operator=(Where<T> &&other) = default;
  };

  template <typename T> class Take : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    size_t remaining;
    const size_t _capacity;

    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    shared_ptr<Functor<T>> deepCopy() const {
      auto copy = make_shared<Take<T>>(_capacity);
      if (previousFunction)
        copy->previousFunction = previousFunction->deepCopy();
      return copy;
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      if (reset)
        remaining = _capacity;
      if (!remaining)
        return nullptr;
      --remaining;
      auto result = previousFunction->operator()(reset);
      if (result == nullptr)
        remaining = 0;
      return result;
    }

  public:
    Take(const size_t &capacity)
        : previousFunction(nullptr), remaining(capacity), _capacity(capacity) {}
    Take(const Take<T> &other)
        : previousFunction(other.previousFunction
                               ? other.previousFunction->deepCopy()
                               : nullptr),
          remaining(other._capacity), _capacity(other._capacity) {}
    Take<T> &operator=(const Take<T> &other) {
      remaining = _capacity = other._capacity;
      previousFunction =
          other.previousFunction ? other.previousFunction->deepCopy() : nullptr;
      return *this;
    }
    Take(Take<T> &&other) = default;
    Take<T> &operator=(Take<T> &&other) = default;
  };

  template <typename T> class OrderBy : public Functor<T> {
    shared_ptr<Functor<T>> previousFunction;
    function<bool(const T &, const T &)> comparer;
    bool processed;
    deque<unique_ptr<T>> results;

    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      this->previousFunction = previousFunction;
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return previousFunction;
    }
    shared_ptr<Functor<T>> deepCopy() const {
      auto copy = make_shared<OrderBy<T>>(comparer);
      if (previousFunction)
        copy->previousFunction = previousFunction->deepCopy();
      return copy;
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

  public:
    OrderBy(const function<bool(const T &, const T &)> &comparer)
        : previousFunction(nullptr), comparer(comparer), processed(false) {}
    OrderBy(const OrderBy<T> &other)
        : previousFunction(other.previousFunction
                               ? other.previousFunction->deepCopy()
                               : nullptr),
          comparer(other.comparer), processed(false) {}
    OrderBy<T> &operator=(const OrderBy<T> &other) {
      comparer = other.comparer;
      processed = false;
      results.clear();
      previousFunction =
          other.previousFunction ? other.previousFunction->deepCopy() : nullptr;
      return *this;
    }
    OrderBy(OrderBy<T> &&other) = default;
    OrderBy<T> &operator=(OrderBy<T> &&other) = default;
  };

  template <typename T> class Composer : public Functor<T> {
    shared_ptr<Functor<T>> first, last;

    class Iterate : public Functor<T> {
      typename vector<T>::const_iterator it, end;

    public:
      template <typename C>
      Iterate(const C &values) : it(cbegin(values)), end(cend(values)) {}
      Iterate(const Iterate &other) = default;
      Iterate(Iterate &&other) = default;
      Iterate &operator=(const Iterate &other) = default;
      Iterate &operator=(Iterate &&other) = default;
      void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {}
      shared_ptr<Functor<T>> getPreviousFunction() const { return nullptr; }
      shared_ptr<Functor<T>> deepCopy() const {
        return make_shared<Iterate>(*this);
      }
      inline unique_ptr<T> operator()(const bool &reset) {
        if (it == end)
          return nullptr;
        auto result = make_unique<T>(*it);
        ++it;
        return result;
      }
    };

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
    void setPreviousFunction(shared_ptr<Functor<T>> previousFunction) {
      last->setPreviousFunction(previousFunction);
    }
    shared_ptr<Functor<T>> getPreviousFunction() const {
      return last->getPreviousFunction();
    }
    shared_ptr<Functor<T>> deepCopy() const {
      auto copy = make_shared<Composer<T>>();
      if (first) {
        copy->last = copy->first = first->deepCopy();
        while (copy->last->getPreviousFunction())
          copy->last = copy->last->getPreviousFunction();
      }
      return copy;
    }
    inline unique_ptr<T> operator()(const bool &reset) {
      return first->operator()(reset);
    }

  public:
    Composer() : first(nullptr), last(nullptr) {}
    Composer(const Composer<T> &other) {
      auto copy = static_pointer_cast<Composer<T>>(other.deepCopy());
      first = move(copy->first);
      last = move(copy->last);
    }
    Composer<T> &operator=(const Composer<T> &other) {
      auto copy = static_pointer_cast<Composer<T>>(other.deepCopy());
      first = move(copy->first);
      last = move(copy->last);
      return *this;
    }
    Composer(Composer<T> &&other) = default;
    Composer<T> &operator=(Composer<T> &&other) = default;
    void clear() { first = last = nullptr; }
    template <typename F> Composer<T> &append(F func) {
      shared_ptr<Functor<T>> temp = make_shared<F>(move(func));
      temp->setPreviousFunction(first);
      first = temp;
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
    template <typename C> inline vector<T> ToList(const C &values) {
      return preprocess(values);
    }
    inline vector<T> ToList(const initializer_list<T> &values) {
      return preprocess(values);
    }
  };

} // namespace Pipeline
