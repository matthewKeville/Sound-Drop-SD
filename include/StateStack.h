#ifndef STATE_STACK_H
#define STATE_STACK_H

#include <vector>
#include "Line.h"
#include "Spawner.h"
#include "SaveState.h"

//undo redo 
struct StateNode {
  StateNode* prev;
  StateNode* next;

  SaveState* state;
};

/* A timeline data structure that obliterates the future if the
 * past was changed.
 */
class StateStack {
  public  :
    StateStack();

    void Forward(); //rollback to next save state
    void Record(SaveState* currentSave);
    void Back();    //roll forward to last save state

    bool HasForward();
    bool HasBack();

    SaveState* Current(); 

  private :
    const unsigned short int size = 5;  //max no. of time points
    unsigned short int activeStateCount; //true time points
    StateNode* future; //the farthest true future
    StateNode* present; 
    StateNode* past;   //the farthest true past
    void RewriteFuture(); //destroy all nodes future to the current
    void RewritePast();
};

#endif
