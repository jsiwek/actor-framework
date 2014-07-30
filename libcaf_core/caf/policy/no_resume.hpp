#ifndef NO_RESUME_HPP
#define NO_RESUME_HPP

#include <chrono>
#include <utility>

#include "caf/exception.hpp"
#include "caf/exit_reason.hpp"
#include "caf/policy/resume_policy.hpp"

namespace caf {
namespace policy {

class no_resume {
 public:
  template <class Base, class Derived>
  struct mixin : Base {
    template <class... Ts>
    mixin(Ts&&... args) : Base(std::forward<Ts>(args)...), m_hidden(true) {
      // nop
    }

    void attach_to_scheduler() {
      this->ref();
    }

    void detach_from_scheduler() {
      this->deref();
    }

    resumable::resume_result resume(execution_unit*) {
      auto done_cb = [=](uint32_t reason) {
        this->planned_exit_reason(reason);
        this->on_exit();
        this->cleanup(reason);
      };
      std::exception_ptr eptr = nullptr;
      try {
        this->act();
        done_cb(exit_reason::normal);
      }
      catch (actor_exited& e) {
        done_cb(e.reason());
      }
      catch (...) {
        eptr = std::current_exception();
      }
      if (eptr) {
        uint32_t reason = exit_reason::unhandled_exception;
        try {
          auto opt_reason = this->handle(eptr);
          if (opt_reason) {
            // use exit reason defined by custom handler
            reason = *opt_reason;
          }
        }
        catch (...) {
          // just ignore it
        }
        done_cb(reason);
      }
      return resumable::done;
    }

    bool m_hidden;
  };

  template <class Actor>
  void await_ready(Actor* self) {
    self->await_data();
  }
};

} // namespace policy
} // namespace caf

#endif // NO_RESUME_HPP
