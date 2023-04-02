#include "Component.h"
#include "Entity.h"

void Component::SetEntity(Entity* entity)
{
    owner_ = entity;
}
