#ifndef INTERACTABLE_H
#define INTERACTABLE_H

class Interactable {
  public :
    virtual ~Interactable(); //virtual method in base -> virtual deconstrcutor
    virtual bool IsHovering(float ndcx,float nxcy) = 0;  //normalized device coordinate pair
    //displace the interactable (it does not set it to the supplied coordinates)
    virtual void move(float x,float y) = 0;
    //position the interactable at the given location
    virtual void position(float x,float y) = 0;
    //?delete?
    void SetInteracting(bool interacting);
    bool IsInteracting();

    bool isDeleted();         //requires deletion?
    void markDeleted();       //set requiresDeletion to true
                             
  private :
    bool interacting = false;
    bool deleted = false;
};

#endif
