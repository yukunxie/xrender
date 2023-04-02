
#include "Model.h"
#include "MeshComponent.h"

Model::Model()
{
    //AddComponment(MeshComponentBuilder::CreateBox());
}

Model::~Model()
{
}

void Model::Tick(float dt)
{
    Entity::Tick(dt);
}

Model* Model::CreateBox()
{
    auto model = new Model();
    auto modelComp = MeshComponentBuilder::CreateBox();
    model->AddComponment(modelComp);
    return model;
}