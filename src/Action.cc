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
#include <config.h>
#include "Utils.h"
#include "EventLoop.h"
#include "Action.h"
#include <stdexcept>
#include <cassert>
#include <cstdio>

Action::~Action() {
}

void Action::done(EventLoop *, ActionList *) {
}

void ActionList::add(Action *a) {
  actions.push_back(a);
}

void ActionList::go() {
  while(actions.size() > 0) {
    trigger();
    eventloop->wait();
  }
}

void ActionList::trigger() {
  bool changed;
  do {
    changed = false;
    for(Action *a: actions) {
      if(a->running)
        continue;
      bool blocked = false;
      for(auto &r: a->resources)
        if(resources.find(r) != resources.end()) {
          blocked = true;
          break;
        }
      if(blocked)
        continue;
      a->running = true;
      for(std::string &r: a->resources)
        resources.insert(r);
      a->go(eventloop, this);
      changed = true;
    }
  } while(changed);
}

void ActionList::completed(Action *a) {
  auto it = std::find(actions.begin(), actions.end(), a);
  if(it != actions.end()) {
    assert(a->running);
    for(std::string &r: a->resources)
      resources.erase(r);
    a->running = false;
    actions.erase(it);
    a->done(eventloop, this);
    trigger();
    return;
  }
  throw std::logic_error("ActionList::completed");
}
