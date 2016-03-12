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

void Action::done(EventLoop *, ActionList *) {
}

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
  bool changed;
  do {
    changed = false;
    for(auto it: actions) {
      Action *a = it.second;
      if(a->running)
        continue;
      bool blocked = false;
      for(auto &p: a->predecessors) {
        if(actions.find(p) != actions.end()) {
          D("action %s blocked by dependency %s",
            a->name.c_str(), p.c_str());
          blocked = true;
          break;
        } else if(complete.find(p) == complete.end())
          throw std::logic_error(a->name + " follows unknown action " + p);
      }
      for(auto &r: a->resources)
        if(contains(resources, r)) {
          D("action %s blocked by resource %s",
            a->name.c_str(), r.c_str());
          blocked = true;
          break;
        }
      if(blocked)
        continue;
      a->running = true;
      for(std::string &r: a->resources)
        resources.insert(r);
      D("action %s starting", a->name.c_str());
      a->go(eventloop, this);
      // 'it' now invalidated
      changed = true;
      break;
    }
  } while(changed);
}

void ActionList::completed(Action *a) {
  D("action %s completed", a->name.c_str());
  auto it = actions.find(a->name);
  if(it != actions.end()) {
    assert(a->running);
    for(std::string &r: a->resources)
      resources.erase(r);
    a->running = false;
    actions.erase(it);
    complete[a->name] = a;
    a->done(eventloop, this);
    trigger();
    return;
  }
  throw std::logic_error("ActionList::completed");
}
