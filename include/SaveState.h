#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <vector>
#include "Line.h"
#include "Spawner.h"

class SaveState {
  private :
    std::vector<Line*> lines;
    std::vector<Spawner*> spawners;
  public :
    //load the parameters into local storage
    void save(std::vector<Line*>& saveLines ,std::vector<Spawner*>& saveSpawners);
    //extract the local storage into the provided parameters
    void load(std::vector<Line*>& liveLines ,std::vector<Spawner*>& liveSpawners);
    void deleteLocal();
    ~SaveState();
};

#endif
