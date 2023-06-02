#ifndef INTERACTABLE_H
#define INTERACTABLE_H

class Interactable {
  public :
    virtual bool IsHovering(float ndcx,float nxcy) = 0;  //normalized device coordinate pair
    //displace the interactable (it does not set it to the supplied coordinates)
    virtual void move(float x,float y) = 0;
    //position the interactable at the given location
    virtual void position(float x,float y) = 0;
    //?delete?
    void SetInteracting(bool interacting);
    bool IsInteracting();
  private :
    bool interacting = false;
};

#endif
