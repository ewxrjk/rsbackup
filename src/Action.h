//-*-C++-*-
// Copyright © 2015 Richard Kettlewell.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#ifndef ACTION_H
#define ACTION_H
/** @file Action.h
 * @brief Sequencing of concurrent operations
 *
 * An @ref Action performs some task, while holding some collection of
 * resources.  The task is executed within the context of an @ref EventLoop and
 * is initiated by @ref Action::go.  Resources are registered using @ref
 * Action::uses.
 *
 * An @ref ActionList is an ordered container of @ref Action objects.  Actions
 * are executed concurrently, with the restriction that no two actions can hold
 * the same resource at concurrently.
 */

#include <list>
#include <set>
#include <string>
#include <vector>

class ActionList;
class EventLoop;

/** @brief One action that may be initiated concurrently
 *
 * An @ref Action performs some task, while holding some collection of
 * resources.  The task is executed within the context of an @ref EventLoop and
 * is initiated by @ref Action::go; it should call ActionList::completed when
 * it is finished.  Resources are registered using @ref Action::uses.

 * Actions must be added to an @ref ActionList to be executed.
 */
class Action {
public:
  /** @brief Constructor
   */
  Action(): running(false) {
  }

  /** @brief Destructor */
  virtual ~Action();

  /** @brief Specify a resource that this action uses
   * @param r Resource name
   */
  void uses(const std::string &r) {
    resources.push_back(r);
  }

  /** @brief Start the action
   * @param e Event loop
   * @param al Execution context
   *
   * Actions should not (normally) block but instead execute asynchronously
   * using event loop @p e.
   *
   * When it is complete it must call @ref ActionList::completed.
   */
  virtual void go(EventLoop *e, ActionList *al) = 0;

  /** @brief Called when the action is complete
   * @param e Event loop
   * @param al Execution context
   *
   * The default implementation does nothing.
   */
  virtual void done(EventLoop *e, ActionList *al);

private:
  friend class ActionList;

  /** @brief Resources required by this action */
  std::vector<std::string> resources;

  /** @brief Current state */
  bool running;
};

/** @brief A collection of actions that are executed concurrently
 *
 * @ref Action "Actions" are executed concurrently, with the restriction that no two actions
 * can hold the same resource concurrently.
 *
 * When a new action is to be executed, the first action that has not been
 * started and does not contradict the restrictions above, is chosen for
 * execution.  Actions are executed via Action::go; they should call
 * @ref ActionList::completed when they are finished.
 */
class ActionList {
public:
  /** @brief Constructor
   * @param e Event loop
   */
  ActionList(EventLoop *e): eventloop(e) {
  }

  /** @brief Add an action
   * @param a Action
   *
   * Adds an action to the end of the list.  @p a must remain valid at least
   * until it has been completed, i.e. until @ref Action::done is called.
   */
  void add(Action *a);

  /** @brief Initiate actions
   *
   * Returns when all actions are complete.
   *
   * This method repeatedly calls @ref EventLoop::wait, so if there are any
   * @ref Reactor objects attached to the event loop that do not belong to some
   * action, unexpected delays may result.
   */
  void go();

  /** @brief Called when an action is complete
   *
   * It is the responsibility of @ref Action subclasses to call this method.
   * Normally it should not leave any @ref Reactor objects attached to the @ref
   * EventLoop when it does so (see the caveat at @ref Action::go).
   */
  void completed(Action *a);

private:
  /** @brief Event loop */
  EventLoop *eventloop;

  /** @brief List of remaining actions */
  std::list<Action *> actions;

  /** @brief Start any new actions if possible */
  void trigger();

  /** @brief In-use resources */
  std::set<std::string> resources;
};

#endif /* ACTION_H */
