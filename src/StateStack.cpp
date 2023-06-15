#include <vector>
#include "SaveState.h"
#include "StateStack.h"
#include "Line.h"
#include "Spawner.h"

StateStack::StateStack() {
  this->future = this->present = this->past = nullptr;
  this->activeStateCount = 0;
}

void StateStack::Record(SaveState* currentState) {
  //does this eliminate the *past* future
  if ( future != present ) {
    RewriteFuture();
  }

  //does this eliminate a past state?
  if ( activeStateCount == size-1 ) {
    RewritePast();
  }

  //no recorded states
  if ( present == nullptr ) {
    present = new StateNode { nullptr, nullptr , currentState };
  } else {
    present->next = new StateNode { present , nullptr , currentState };
    present = present->next;
    future = present;
  }
  activeStateCount++;
}

bool StateStack::HasForward() {
  //short circuit avoid null reference
  return (present!=nullptr && present->next != nullptr);
}

bool StateStack::HasBack() {
  //short circuit avoid null reference
  return (present!=nullptr && present->prev != nullptr);
}

void StateStack::Forward() {
  if (HasForward()) {
    present = present->next;
  }
}

void StateStack::Back() {
  if (HasBack()) {
    present = present->prev;
  }
}

//delete the oldest entry
void StateStack::RewritePast() {
  StateNode* newPast = past->next;
  newPast->prev = nullptr; //2nd to last past needs updating
  delete past; //warning : this deletes a pointer to pointers
  past = newPast;
  activeStateCount--;
}

//delete all entries future to the present
void StateStack::RewriteFuture() {
  short int deletes = 1;
  StateNode* sn = future->prev;
  while ( sn != present ) {
    delete sn->next; 
    sn = sn->prev;
    deletes++;
  }
  delete present->next;
  present->next = nullptr;
  future = present;
  activeStateCount-=deletes;
}

SaveState* StateStack::Current() {
  return present == nullptr ? nullptr : present->state;
}


