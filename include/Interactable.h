#ifndef INTERACTABLE_H
#define INTERACTABLE_H

#include <glm/glm.hpp>

class Interactable {
  public :
    virtual ~Interactable(); //virtual method in base -> virtual deconstrcutor
                             
    virtual bool IsHovering(/* world space coordinate pair*/glm::vec2 wscp) = 0;
    virtual void move(float x,float y) = 0;       //displace the interactable (it does not set it to the supplied coordinates)
    virtual void position(float x,float y) = 0;   //position the interactable at the given location
    virtual glm::vec2 getPosition() = 0;
                                                 
    //void SetInteracting(bool interacting);
    //bool IsInteracting();
    bool isDeleted();         //requires deletion?
    void markDeleted();       //set requiresDeletion to true
  private :
    bool interacting = false;
    bool deleted = false;
};

#endif
