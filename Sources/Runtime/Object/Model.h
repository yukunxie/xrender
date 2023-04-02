#pragma once

#include "Entity.h"
//#include "Meshes/Mesh.h"

class Model : public Entity
{
public:
    Model();
    virtual ~Model();

public:
    static Model* CreateBox();

public:
    
    virtual void Tick(float dt) override;
};
