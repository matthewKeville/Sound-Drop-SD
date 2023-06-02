#ifndef INTERACTABLE_H
#define INTERACTABLE_H

class Interactable {
  public :
    virtual bool IsHovering(float ndcx,float nxcy) = 0;  //normalized device coordinate pair
};

#endif
