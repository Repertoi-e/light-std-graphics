#pragma once

#include "lstd/array.h"

LSTD_BEGIN_NAMESPACE

// @TODO: Rename back to signal once we get rid of C naming conflicts
template <typename Signature>
struct delegate_signal;

template <typename R, typename... Args>
struct delegate_signal<R(Args...)> {
  using result_t = R;
  using callback_t = delegate<R(Args...)>;

  // @ThreadSafety: Lock-free linked list?
  array<callback_t> Callbacks;

  bool CurrentlyEmitting = false;
  array<s64> ToRemove;
};

// Add a new callback, returns a handler ID which you can use to remove the
// callback later
template <typename F>
s64 connect(delegate_signal<F> ref s, delegate<F> cb) {
  reserve(s.Callbacks);
  if (cb) add(s.Callbacks, cb);
  return s.Callbacks.Count - 1;
}

// Remove a callback via connection id. Returns true on success.
template <typename F>
bool disconnect(delegate_signal<F> ref s, s64 index) {
  if (!s.CurrentlyEmitting) {
    assert(index <= s.Callbacks.Count);
    if (s.Callbacks[index]) {
      s.Callbacks[index] = null;
      return true;
    }
    return false;
  }
  add(s.ToRemove, index);
  return true;  // We will remove the callback once we have finished emitting
}

template <typename T>
void handle_to_remove_after_emit_end(delegate_signal<T> ref s) {
  For(s.ToRemove) {
    assert(it <= s.Callbacks.Count);
    if (s.Callbacks[it]) s.Callbacks[it] = null;
  }
  s.ToRemove.Count = 0;
}

// Emits to all callbacks
template <typename R, typename... Args>
void emit(delegate_signal<R(Args...)> ref s, Args no_copy... args) {
  s.CurrentlyEmitting = true;
  For(s.Callbacks) if (it) it((Args no_copy)args...);
  s.CurrentlyEmitting = false;
  handle_to_remove_after_emit_end(s);
}

// Calls registered callbacks until one returns true.
// Used for e.g. window events - when the user clicks on the UI the event should
// not be propagated to the world.
template <typename R, typename... Args>
void emit_while_false(delegate_signal<R(Args...)> ref s, Args... args) {
  static_assert(is_convertible<R, bool>);

  s.CurrentlyEmitting = true;
  For(s.Callbacks) if (it) if (it((Args no_copy)args...)) break;
  s.CurrentlyEmitting = false;
  handle_to_remove_after_emit_end(s);
}

// Calls registered callbacks until one returns false
template <typename R, typename... Args>
void emit_while_true(delegate_signal<R(Args...)> ref s, Args... args) {
  static_assert(is_convertible<R, bool>);

  s.CurrentlyEmitting = true;
  For(s.Callbacks) if (it) if (!it((Args no_copy)args...)) break;
  s.CurrentlyEmitting = false;
  handle_to_remove_after_emit_end(s);
}

template <typename T>
void free(delegate_signal<T> ref s) {
  // @Cleanup Make it a stack array
  free(s.Callbacks);
  free(s.ToRemove);
}

LSTD_END_NAMESPACE
