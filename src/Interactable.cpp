#include "Interactable.h"

Interactable::~Interactable() {
  //no non-primitives to delete
}

/*
void Interactable::SetInteracting(bool interacting) {
  this->interacting = interacting;
}

bool Interactable::IsInteracting() {
  return interacting;
}
*/

bool Interactable::isDeleted() {
  return deleted;
}

void Interactable::markDeleted() {
  deleted = true;
}
