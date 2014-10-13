/*
 * A small variadic implementation of scopeguard
 */
#include <functional>


/*
 * Class: scopeguard_base
 *
 * This exists mainly so we can have a non-templated base for references.
 */
class scopeguard_base
{
public:
    void dismiss() const {
        this->dismissed_ = true;
    }
protected:
    scopeguard_base() : dismissed_(false) {}
    scopeguard_base(scopeguard_base const& other)
        : dismissed_(other.dismissed_) {
        other.dismiss();
    }
    // Note: no virtual! We don't invoke delete polymorphically, instead
    // relying on the extension of const-ref-to-temporary lifetime.
    ~scopeguard_base() {}
    mutable bool dismissed_;
private:
    scopeguard_base& operator=(scopeguard_base const&);
};


/*
 * Class: scopeguard_impl
 *
 * The primary implementation of scopeguard, supports arbitrary
 * non-member n-ary functions, functors, lambdas, etc.
 */
template <typename F, typename... P>
class scopeguard_impl : public scopeguard_base
{
    std::function<void()> func_;
public:
    scopeguard_impl(F const& f, P&&... args)
        : func_([&]() { f(std::forward<P>(args)...); })
    { }
    ~scopeguard_impl() {
        this->func_();
    }
};

/*
 * Function: make_guard
 *
 * Convenient helper, allowing template type deduction to streamline
 * the process of creating a scopeguard_impl object.
 */
template <typename F, typename... P>
scopeguard_impl<F, P...> make_guard(F const& f, P&&... args) {
    return scopeguard_impl<F, P...>(f, std::forward<P>(args)...);
}

typedef scopeguard_base const& scopeguard;

