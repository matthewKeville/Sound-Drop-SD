#include "Interactable.h"

void Interactable::SetInteracting(bool interacting) {
  this->interacting = interacting;
}

bool Interactable::IsInteracting() {
  return interacting;
}
