#include <vector>
#include "SaveState.h"
#include "Line.h"
#include "Spawner.h"

void SaveState::save(std::vector<Line*>& lines ,std::vector<Spawner*>& spawners) {
  //delete pointed to lines
  for ( auto lp : this->lines ) {
    delete lp;
  }
  //delete pointed to spawners
  for ( auto sp : this->spawners ) {
    delete sp;
  }
  this->lines.clear();
  this->spawners.clear();
  //for each line in lines we need a true copy of the line
  for ( auto lp : lines ) {
    Line* nlp = new Line {*lp};
    this->lines.push_back(nlp);
  }
  for ( auto sp : spawners ) {
    Spawner* nsp = new Spawner {*sp};
    this->spawners.push_back(nsp);
  }
}

void SaveState::load(std::vector<Line*>& liveLines ,std::vector<Spawner*>& liveSpawners) {
  //delete pointed to lines
  for ( auto lp : liveLines ) {
    delete lp;
  }
  //delete pointed to spawners
  for ( auto sp : liveSpawners ) {
    delete sp;
  }
  //like above, we want to provide an array of unique pointers to a unique copy
  //of the data, otherwise we will delete the underlying container
  liveLines.clear();
  liveSpawners.clear();
  
  for ( auto lp : this->lines ) {
    Line* nlp = new Line {*lp};
    liveLines.push_back(nlp);
  }
  for ( auto sp : this->spawners ) {
    Spawner* nsp = new Spawner {*sp};
    liveSpawners.push_back(nsp);
  }

}

