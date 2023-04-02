#pragma once

#include "Types.h"


class Entity;

class Component
{
public:
    void SetEntity(Entity* entity);

    Entity* GetOwner()
    {
        return owner_;
    }

    virtual void Tick(float dt) {}

protected:
    Entity* owner_ = nullptr;
};

class Transform : Component
{
public:
    TVector3& Position() const
    {
        return position_;
    }

    void SetPosition(const TVector3& position)
    {
        position_ = position;
    }

    TVector3& Scale() const
    {
        return scale_;
    }

    void SetScale(const TVector3& scale)
    {
        scale_ = scale;
    }

    TVector3& Rotation() const
    {
        return rotation_;
    }

    void SetRotation(const TVector3& rotation)
    {
        rotation_ = rotation;
    }
    
protected:
    mutable TVector3 position_ = { 0, 0, 0 };
    mutable TVector3 scale_ = { 1, 1, 1 };
    mutable TVector3 rotation_ = { 0, 0, 0 };
};
