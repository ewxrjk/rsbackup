//-*-C++-*-
// Copyright © 2015, 2016 Richard Kettlewell.
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
 * the same resource concurrently.
 *
 * These objects are (intended to be) used wherever concurrency can be
 * exploited.  Currently, this means @ref pruneBackups and @ref retireVolumes.
 */

#include <set>
#include <string>
#include <vector>
#include <map>

class ActionList;
class EventLoop;

/** @brief Status of an action */
struct ActionStatus {
  /** @brief Action name */
  std::string name;

  /** @brief Action flags
   *
   * @see Action::after
   */
  unsigned flags;

};

/** @brief Action must succeed
 *
 * @see Action::after
 */
#define ACTION_SUCCEEDED 0x0001

/** @brief Match action by globbing
 *
 * @see Action::after
 */
#define ACTION_GLOB 0x0002

/** @brief One action that may be initiated concurrently
 *
 * An @ref Action performs some task, while holding some collection of
 * resources.  The task is executed within the context of an @ref EventLoop and
 * is initiated by @ref Action::go; it should call ActionList::completed when
 * it is finished.
 *
 * Actions require <i>resources</i>, which are identified by strings.  No two
 * actions that require the same resource (as identified by string comparison)
 * are run concurrently.  Resources are registered using @ref Action::uses.
 *
 * Actions have <i>dependencies</i> on other actions, identified either by
 * strings or by glob patterns.  Furthermore the dependency may either be an
 * order-only dependency, which just controls sequencing, or may be a
 * dependency on the success of an action.  Dependencies are registered using
 * @ref Action::after.
 *
 * Actions must be added to an @ref ActionList to be executed.
 */
class Action {
public:
  /** @brief Constructor
   * @param name Action name
   */
  Action(const std::string &name): name(name) {}

  /** @brief Destructor */
  virtual ~Action() = default;

  /** @brief Specify a resource that this action uses
   * @param r Resource name
   *
   * Actions that use the same resource are not run concurrently.
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

  /** @brief Add a constraint that this action must follow another
   * @param name Name of action that this action must follow
   * @param flags Flags word
   *
   * The possible flag values are:
   *
   * <table>
   * <tr><th>@p flags value</th><th>Meaning</th></tr>
   * <tr><td>@c 0</td><td>@p name will be treated as an action name, which must
   * exist.  This action will run after it and will only run if after @p name
   * has completed, whether or not it succeeded.</td></tr>
   * <tr><td>@c ACTION_SUCCEEDED</td><td>@p name will be treated as an action
   * name, which must exist.  This action will run after it and will only run
   * if it @p name succeeded; if it failed, then this action will also fail,
   * without running.</td></tr>
   * <tr><td>@c ACTION_GLOB</td><td>@p name will be treated as a glob pattern.
   * This action will run after all matching actions (if there are any) have
   * completed, whether or not they succeeded.</td></tr>
   * <tr><td>@c ACTION_SUCCEEDED|ACTION_GLOB</td><td>@p name will be treated as
   * a glob pattern.  This action will run after all matching actions (if there
   * are any) have succeeded.  If at least one of them fails then this action
   * will also fail, without running, after they have all completed.</td></tr>
   * </table>
   *
   * Glob matching is done via <b>fnmatch</b>(3) and has the @c FNM_PATHNAME
   * flag set.
   */
  void after(const std::string &name, unsigned flags) {
    predecessors.push_back({name, flags});
  }

  /** @brief Set the action's priority
   * @param p New action priority
   *
   * Actions are dispatched in descending order of priority. The default
   * priority for a new action is 0.
   */
  void set_priority(int p) {
    priority = p;
  }

private:
  friend class ActionList;

  /** @brief Name of action */
  std::string name;

  /** @brief Resources required by this action */
  std::vector<std::string> resources;

  /** @brief Predecessors of this action */
  std::vector<ActionStatus> predecessors;

  /** @brief Current state */
  bool running = false;

  /** @brief Priority */
  int priority = 0;
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
   * @param wait_for_timeouts Whether event loop should wait for timeouts
   *
   * Returns when all actions are complete.
   *
   * This method repeatedly calls @ref EventLoop::wait, so if there are any
   * @ref Reactor objects attached to the event loop that do not belong to some
   * action, unexpected delays may result.
   */
  void go(bool wait_for_timeouts = false);

  /** @brief Called when an action is complete
   * @param a Action that completed
   * @param succeeded @c true if the action succeeded, otherwise false
   *
   * It is the responsibility of @ref Action subclasses to call this method.
   * Normally it should not leave any @ref Reactor objects attached to the @ref
   * EventLoop when it does so (see the caveat at @ref Action::go).
   */
  void completed(Action *a, bool succeeded);

private:
  /** @brief Event loop */
  EventLoop *eventloop;

  /** @brief Remaining actions
   *
   * Includes in-progress actions.
   */
  std::map<std::string, Action *> actions;

  /** @brief Status of completed actions */
  std::map<std::string, bool> status;

  /** @brief Start any new actions if possible */
  void trigger();

  /** @brief In-use resources */
  std::set<std::string> resources;

  /** @brief Called when an action is complete or skipped
   * @param a Action that completed
   * @param succeeded @c true if the action succeeded, otherwise false
   * @param ran @c true if the action actually ran
   */
  void cleanup(Action *a, bool succeeded, bool ran);

  /** @brief Test whether an action is blocked by resource contention
   * @param a Action to check
   * @return @c true if action is blocked
   */
  bool blocked_by_resource(const Action *a);

  /** @brief Test whether an action is blocked by a dependency
   * @param a Action to check
   * @return @c true if action is blocked
   */
  bool blocked_by_dependency(const Action *a);

  /** @brief Test whether an action is failed by a dependency
   * @param a Action to check
   * @return @c true if action is failed
   */
  bool failed_by_dependency(const Action *a);

  /** @brief Find an action by name or pattern
   * @param as Action name or pattern to find
   * @return Iterator pointing to action
   */
  std::map<std::string, Action *>::iterator find(const ActionStatus &as);
};

#endif /* ACTION_H */
