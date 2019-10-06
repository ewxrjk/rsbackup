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
#include <config.h>
#include "Utils.h"
#include "EventLoop.h"
#include "Action.h"
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <cstdio>
#include <fnmatch.h>

void Action::done(EventLoop *, ActionList *) {}

void ActionList::add(Action *a) {
  if(actions.find(a->name) != actions.end())
    throw std::logic_error("duplicate action " + a->name);
  actions[a->name] = a;
}

void ActionList::go(bool wait_for_timeouts) {
  D("go");
  while(actions.size() > 0) {
    trigger();
    eventloop->wait(wait_for_timeouts);
  }
}

void ActionList::trigger() {
  D("trigger");
  Action *chosen = nullptr;
  for(auto it: actions) {
    Action *a = it.second;
    if(a->running || blocked_by_resource(a) || blocked_by_dependency(a))
      continue;
    if(failed_by_dependency(a)) {
      cleanup(a, false, false);
      return trigger();
    }
    if(chosen == nullptr || chosen->priority < a->priority)
      chosen = a;
  }
  if(chosen) {
    chosen->running = true;
    for(std::string &r: chosen->resources)
      resources.insert(r);
    D("action %s starting", chosen->name.c_str());
    chosen->go(eventloop, this);
    // Repeat in case there are more
    return trigger();
  }
}

void ActionList::completed(Action *a, bool succeeded) {
  cleanup(a, succeeded, true);
}

void ActionList::cleanup(Action *a, bool succeeded, bool ran) {
  D("action %s %s", a->name.c_str(), succeeded ? "succeeded" : "failed");
  auto it = actions.find(a->name);
  if(it != actions.end()) {
    assert(a == it->second);
    if(ran) {
      assert(a->running);
      for(std::string &r: a->resources)
        resources.erase(r);
      a->running = false;
    }
    actions.erase(it);
    status[a->name] = succeeded;
    if(ran) {
      a->done(eventloop, this);
      trigger();
    }
    return;
  }
  throw std::logic_error("ActionList::cleanup");
}

bool ActionList::blocked_by_resource(const Action *a) {
  for(auto &r: a->resources)
    if(contains(resources, r)) {
      D("action %s blocked by resource %s", a->name.c_str(), r.c_str());
      return true;
    }
  return false;
}

bool ActionList::failed_by_dependency(const Action *a) {
  for(auto &p: a->predecessors) {
    if(p.flags & ACTION_GLOB) {
      for(auto it: status) {
        if(fnmatch(p.name.c_str(), it.first.c_str(), FNM_PATHNAME)
               != FNM_NOMATCH
           && it.second == false) {
          D("action %s depends on success of failed action %s as %s",
            a->name.c_str(), it.first.c_str(), p.name.c_str());
          return true;
        }
      }
    } else {
      auto d = status.find(p.name);
      if(d != status.end()               // P completed or failed
         && (p.flags & ACTION_SUCCEEDED) // A needs P to have succeeded
         && !d->second) {                // P failed
        D("action %s depends on success of failed action %s", a->name.c_str(),
          p.name.c_str());
        return true;
      }
    }
  }
  return false;
}

bool ActionList::blocked_by_dependency(const Action *a) {
  for(auto &p: a->predecessors) {
    if(find(p) != actions.end()) {
      D("action %s blocked by dependency %s", a->name.c_str(), p.name.c_str());
      return true;
    } else { // Sanity check
      if(!(p.flags & ACTION_GLOB)) {
        auto d = status.find(p.name);
        if(d == status.end())
          throw std::logic_error(a->name + " follows unknown action " + p.name);
      }
    }
  }
  return false;
}

std::map<std::string, Action *>::iterator
ActionList::find(const ActionStatus &as) {
  if(as.flags & ACTION_GLOB) {
    auto it = actions.begin();
    while(it != actions.end()
          && fnmatch(as.name.c_str(), it->first.c_str(), FNM_PATHNAME)
                 == FNM_NOMATCH)
      ++it;
    return it;
  } else
    return actions.find(as.name);
}
