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

#ifndef CAF_DETAIL_SINGLETONS_HPP
#define CAF_DETAIL_SINGLETONS_HPP

#include <mutex>
#include <atomic>
#include <cstddef> // size_t

#include "caf/fwd.hpp"

namespace caf {
namespace detail {

class abstract_singleton {

 public:

  virtual ~abstract_singleton();

  virtual void dispose() = 0;

  virtual void stop() = 0;

  virtual void initialize() = 0;

};

class singletons {
 public:
  template<typename T>
  struct container {
    std::atomic<T*> ptr;
    std::mutex mtx;
  };

  singletons() = delete;

  static constexpr size_t max_plugins = 3;

  static constexpr size_t middleman_plugin_id = 0;   // io lib

  static constexpr size_t opencl_plugin_id = 1;      // OpenCL lib

  static constexpr size_t probe_plugin_id = 2;       // probe hooks

  static logging* get_logger();

  static node_id get_node_id();

  static scheduler::abstract_coordinator* get_scheduling_coordinator();

  // returns false if singleton is already defined
  static bool set_scheduling_coordinator(scheduler::abstract_coordinator*);

  static group_manager* get_group_manager();

  static actor_registry* get_actor_registry();

  static uniform_type_info_map* get_uniform_type_info_map();

  static message_data* get_tuple_dummy();

  // usually guarded by implementation-specific singleton getter
  template <class Factory>
  static abstract_singleton* get_plugin_singleton(size_t id, Factory f) {
    return lazy_get(get_plugin_singleton(id), f);
  }

  static void stop_singletons();

 private:
  static container<abstract_singleton>& get_plugin_singleton(size_t id);

  // Get instance from @p ptr or crate it on-the-fly using DCLP
  template <class T, class Factory>
  static T* lazy_get(container<T>& data, Factory f) {
    auto result = data.ptr.load(std::memory_order_acquire);
    if (result == nullptr) {
      std::lock_guard<std::mutex> guard(data.mtx);
      result = data.ptr.load(std::memory_order_relaxed);
      if (result == nullptr) {
        result = f();
        result->initialize();
        data.ptr.store(result, std::memory_order_release);
      }
    }
    return result;
  }

  template <class T>
  static T* lazy_get(container<T>& data) {
    return lazy_get(data, [] { return T::create_singleton(); });
  }

  template <class T>
  static void stop(container<T>& data) {
    auto p = data.ptr.load();
    if (p) p->stop();
  }

  template <class T>
  static void dispose(container<T>& data) {
    auto p = data.ptr.load();
    for (;;) {
      if (p == nullptr) {
        return;
      }
      if (data.ptr.compare_exchange_weak(p, nullptr)) {
        p->dispose();
        return;
      }
    }
  }
};

} // namespace detail
} // namespace caf

#endif // CAF_DETAIL_SINGLETONS_HPP
