// Copyright © 2016 Richard Kettlewell.
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

static int action_number;

class SimpleAction: public Action {
public:
  SimpleAction(const std::string &n): Action(n) {
  }

  void go(EventLoop *, ActionList *al) override {
    acted = ++action_number;
    al->completed(this, true);
  }

  int acted = 0;
};

static void test_action_simple() {
  SimpleAction a1("a1"), a2("a2"), a3("a3");
  EventLoop e;
  ActionList al(&e);
  al.add(&a1);
  al.add(&a2);
  al.add(&a3);
  al.go();
  assert(a1.acted);
  assert(a2.acted);
  assert(a3.acted);
}

class SlowAction: public Action, public Reactor {
public:
  SlowAction(const std::string &n, bool outcome = true):
    Action(n), outcome(outcome) {
  }

  void check() {
    for(auto a: require_not_acting)
      assert(!a->acting);
    for(auto a: require_complete)
      assert(a->acted);
    for(auto a: require_not_complete)
      assert(!a->acted);
  }

  void go(EventLoop *e, ActionList *al) override {
    check();
    acting = true;
    this->al = al;
    struct timespec now;
    getMonotonicTime(now);
    now.tv_nsec += 10*10000000;
    now.tv_sec += now.tv_nsec / 1000000000;
    now.tv_sec %= 1000000000;
    e->whenTimeout(now, this);
  }

  void onTimeout(EventLoop *, const struct timespec &) override {
    check();
    acting = false;
    acted = ++action_number;
    al->completed(this, outcome);
  }

  bool acting = false;
  int acted = 0;
  bool outcome;
  ActionList *al;
  std::vector<SlowAction *> require_not_acting;
  std::vector<SlowAction *> require_complete;
  std::vector<SlowAction *> require_not_complete;
};

static void test_action_resources() {
  SlowAction a1("a1"), a2("a2"), a3("a3");
  EventLoop e;
  ActionList al(&e);

  al.add(&a1);
  a1.uses("r1");

  a1.require_not_acting.push_back(&a2);
  al.add(&a2);

  a2.uses("r1");
  a2.require_not_acting.push_back(&a1);

  al.add(&a3);
  a3.uses("r2");
  al.go(true);
  assert(a1.acted);
  assert(a2.acted);
  assert(a3.acted);
  assert(!a1.acting);
  assert(!a2.acting);
  assert(!a3.acting);
}

static void test_action_dependencies() {
  SlowAction a1("a1"), a2("a2"), a3("a3");
  EventLoop e;
  ActionList al(&e);

  al.add(&a1);
  a1.after("a2", 0);
  a1.require_not_acting.push_back(&a2);
  a1.require_not_acting.push_back(&a3);
  a1.require_complete.push_back(&a2);
  a1.require_complete.push_back(&a3);

  al.add(&a2);
  a2.after("a3", 0);
  a2.require_not_acting.push_back(&a1);
  a2.require_not_acting.push_back(&a3);
  a2.require_complete.push_back(&a3);
  a2.require_not_complete.push_back(&a1);

  al.add(&a3);
  a3.require_not_acting.push_back(&a1);
  a3.require_not_acting.push_back(&a2);
  a2.require_not_complete.push_back(&a1);
  a2.require_not_complete.push_back(&a2);

  al.go(true);
  assert(a1.acted);
  assert(a2.acted);
  assert(a3.acted);
  assert(!a1.acting);
  assert(!a2.acting);
  assert(!a3.acting);
}

static void test_action_status() {
  SlowAction a1("a1"), a2("a2", false), a3("a3", false);
  EventLoop e;
  ActionList al(&e);

  al.add(&a1);
  a1.after("a2", ACTION_SUCCEEDED);
  a1.require_not_acting.push_back(&a2);
  a1.require_not_acting.push_back(&a3);
  a1.require_complete.push_back(&a2);
  a1.require_complete.push_back(&a3);

  al.add(&a2);
  a2.after("a3", 0);
  a2.require_not_acting.push_back(&a1);
  a2.require_not_acting.push_back(&a3);
  a2.require_complete.push_back(&a3);
  a2.require_not_complete.push_back(&a1);

  al.add(&a3);
  a3.require_not_acting.push_back(&a1);
  a3.require_not_acting.push_back(&a2);
  a2.require_not_complete.push_back(&a1);
  a2.require_not_complete.push_back(&a2);

  al.go(true);
  assert(!a1.acted);
  assert(a2.acted);
  assert(a3.acted);
  assert(!a1.acting);
  assert(!a2.acting);
  assert(!a3.acting);
}

static void test_action_glob() {
  SlowAction m1("middle/1"), m2("middle/2");
  SlowAction s("start");
  SlowAction l("last");
  EventLoop e;
  ActionList al(&e);

  al.add(&s);
  s.require_not_acting.push_back(&m1);
  s.require_not_acting.push_back(&m2);
  s.require_not_acting.push_back(&l);
  s.require_not_complete.push_back(&m1);
  s.require_not_complete.push_back(&m2);
  s.require_not_complete.push_back(&l);

  al.add(&m1);
  m1.after("start", ACTION_SUCCEEDED);
  m1.require_complete.push_back(&s);
  m1.require_not_acting.push_back(&l);
  m1.require_not_complete.push_back(&l);

  al.add(&m2);
  m2.after("start", ACTION_SUCCEEDED);
  m2.require_complete.push_back(&s);
  m2.require_not_acting.push_back(&l);
  m2.require_not_complete.push_back(&l);

  al.add(&l);
  l.after("middle/*", ACTION_SUCCEEDED|ACTION_GLOB);
  l.require_complete.push_back(&s);
  l.require_complete.push_back(&m1);
  l.require_complete.push_back(&m2);

  al.go(true);
  assert(s.acted);
  assert(m1.acted);
  assert(m2.acted);
  assert(l.acted);
}

static void test_action_glob_status() {
  SlowAction m1("middle/1"), m2("middle/2", false);
  SlowAction s("start", false);
  SlowAction l("last");
  EventLoop e;
  ActionList al(&e);

  al.add(&s);
  s.require_not_acting.push_back(&m1);
  s.require_not_acting.push_back(&m2);
  s.require_not_acting.push_back(&l);
  s.require_not_complete.push_back(&m1);
  s.require_not_complete.push_back(&m2);
  s.require_not_complete.push_back(&l);

  al.add(&m1);
  m1.after("start", 0);
  m1.require_complete.push_back(&s);
  m1.require_not_acting.push_back(&l);
  m1.require_not_complete.push_back(&l);

  al.add(&m2);
  m2.after("start", 0);
  m2.require_complete.push_back(&s);
  m2.require_not_acting.push_back(&l);
  m2.require_not_complete.push_back(&l);

  al.add(&l);
  l.after("middle/*", ACTION_SUCCEEDED|ACTION_GLOB);
  l.require_complete.push_back(&s);
  l.require_complete.push_back(&m1);
  l.require_complete.push_back(&m2);

  al.go(true);
  assert(s.acted);
  assert(m1.acted);
  assert(m2.acted);
  assert(!l.acted);
}

static void test_action_priority() {
  EventLoop e;
  ActionList al(&e);
  SimpleAction a("a"), b("b"), c("c"), d("d");
  a.set_priority(1);
  b.set_priority(2);
  c.set_priority(3);
  d.set_priority(4);
  al.add(&b);
  al.add(&c);
  al.add(&d);
  al.add(&a);
  action_number = 0;
  al.go(true);
  assert(a.acted == 4);
  assert(b.acted == 3);
  assert(c.acted == 2);
  assert(d.acted == 1);
}

int main() {
  //debug = true;
  test_action_simple();
  test_action_resources();
  test_action_dependencies();
  test_action_status();
  test_action_glob();
  test_action_glob_status();
  test_action_priority();
  return 0;
}
