/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENCE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef CAF_ABSTRACT_ACTOR_HPP
#define CAF_ABSTRACT_ACTOR_HPP

#include <set>
#include <mutex>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>

#include "caf/node_id.hpp"
#include "caf/fwd.hpp"
#include "caf/attachable.hpp"
#include "caf/message_id.hpp"
#include "caf/exit_reason.hpp"
#include "caf/intrusive_ptr.hpp"
#include "caf/abstract_channel.hpp"

#include "caf/detail/type_traits.hpp"

namespace caf {

class actor_addr;
class serializer;
class deserializer;
class execution_unit;

/**
 * A unique actor ID.
 * @relates abstract_actor
 */
using actor_id = uint32_t;

/**
 * Denotes an ID that is never used by an actor.
 */
constexpr actor_id invalid_actor_id = 0;

class actor;
class abstract_actor;
class response_promise;

using abstract_actor_ptr = intrusive_ptr<abstract_actor>;

/**
 * Base class for all actor implementations.
 */
class abstract_actor : public abstract_channel {
  // needs access to m_host
  friend class response_promise;
  // java-like access to base class
  using super = abstract_channel;

 public:
  /**
   * Attaches `ptr` to this actor. The actor will call `ptr->detach(...)` on
   * exit, or immediately if it already finished execution.
   * @returns `true` if `ptr` was successfully attached to the actor,
   *          otherwise (actor already exited) `false`.
   */
  bool attach(attachable_ptr ptr);

  /**
   * Convenience function that attaches the functor `f` to this actor. The
   * actor executes `f()` on exit or immediatley if it is not running.
   * @returns `true` if `f` was successfully attached to the actor,
   *          otherwise (actor already exited) `false`.
   */
  template <class F>
  bool attach_functor(F f) {
    struct functor_attachable : attachable {
      F m_functor;
      functor_attachable(F arg) : m_functor(std::move(arg)) {
        // nop
      }
      void actor_exited(uint32_t reason) {
        m_functor(reason);
      }
    };
    return attach(attachable_ptr{new functor_attachable(std::move(f))});
  }

  /**
   * Returns the logical actor address.
   */
  actor_addr address() const;

  /**
   * Detaches the first attached object that matches `what`.
   */
  void detach(const attachable::token& what);

  /**
   * Links this actor to `whom`.
   */
  virtual void link_to(const actor_addr& whom);

  /**
   * Links this actor to `whom`.
   */
  template <class ActorHandle>
  void link_to(const ActorHandle& whom) {
    link_to(whom.address());
  }

  /**
   * Unlinks this actor from `whom`.
   */
  virtual void unlink_from(const actor_addr& whom);

  /**
   * Unlinks this actor from `whom`.
   */
  template <class ActorHandle>
  void unlink_from(const ActorHandle& whom) {
    unlink_from(whom.address());
  }

  /**
   * Establishes a link relation between this actor and `other`
   * and returns whether the operation succeeded.
   */
  virtual bool establish_backlink(const actor_addr& other);

  /**
   * Removes the link relation between this actor and `other`
   * and returns whether the operation succeeded.
   */
  virtual bool remove_backlink(const actor_addr& other);

  /**
   * Returns the unique ID of this actor.
   */
  inline uint32_t id() const {
    return m_id;
  }

  /**
   * Returns the actor's exit reason or
   * `exit_reason::not_exited` if it's still alive.
   */
  inline uint32_t exit_reason() const {
    return m_exit_reason;
  }

  /**
   * Returns the type interface as set of strings or an empty set
   * if this actor is untyped.
   */
  virtual std::set<std::string> interface() const;

 protected:
  /**
   * Creates a non-proxy instance.
   */
  abstract_actor();

  /**
   * Creates a proxy instance for a proxy running on `nid`.
   */
  abstract_actor(actor_id aid, node_id nid);

  /**
   * Called by the runtime system to perform cleanup actions for this actor.
   * Subtypes should always call this member function when overriding it.
   */
  virtual void cleanup(uint32_t reason);

  /**
   * The default implementation for `link_to()`.
   */
  bool link_to_impl(const actor_addr& other);

  /**
   * The default implementation for {@link unlink_from()}.
   */
  bool unlink_from_impl(const actor_addr& other);

  /**
   * Returns `exit_reason() != exit_reason::not_exited`.
   */
  inline bool exited() const {
    return exit_reason() != exit_reason::not_exited;
  }

  /**
   * Returns the execution unit currently used by this actor.
   */
  inline execution_unit* host() const {
    return m_host;
  }

  /**
   * Sets the execution unit for this actor.
   */
  inline void host(execution_unit* new_host) {
    m_host = new_host;
  }

  /** @cond PRIVATE */
  /*
   * Tries to run a custom exception handler for `eptr`.
   */
  optional<uint32_t> handle(const std::exception_ptr& eptr);
  /** @endcond */

 private:
  // cannot be changed after construction
  const actor_id m_id;

  // you're either a proxy or you're not
  const bool m_is_proxy;

  // initially exit_reason::not_exited
  std::atomic<uint32_t> m_exit_reason;

  // guards access to m_exit_reason, m_attachables, and m_links
  std::mutex m_mtx;

  // links to other actors
  std::vector<abstract_actor_ptr> m_links;

  // attached functors that are executed on cleanup
  std::vector<attachable_ptr> m_attachables;

  // identifies the execution unit this actor is currently executed by
  execution_unit* m_host;
};

} // namespace caf

#endif // CAF_ABSTRACT_ACTOR_HPP
