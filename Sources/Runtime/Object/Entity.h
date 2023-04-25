
#pragma once

#include "Types.h"

#include "Component.h"

#include <list>

// ��C++����һ�꣬�Զ��������г�Ա������getter, setter����
#define GETSET(type, varName, funName)        \
private:                                      \
	type varName_;                            \
                                              \
public:                                       \
	inline type Get##funName(void) const      \
	{                                         \
		return varName_;                      \
	}                                         \
                                              \
public:                                       \
	inline void Set##funName(const type& var) \
	{                                         \
		varName_ = var;                       \
	}

// ��C++����һ�꣬�Զ��������г�Ա������getter, setter����
#define GETSET_BOOL(VarName)            \
public:                                 \
	inline bool Is##VarName(void) const \
	{                                   \
		return Is##VarName##_;          \
	}                                   \
                                        \
public:                                 \
	inline void Set##VarName(bool var)  \
	{                                   \
		Is##VarName##_ = var;           \
	}                                   \
                                        \
protected:                              \
	bool Is##VarName##_


class MeshComponent;

struct RTInstanceData
{
	uint32						InstanceId;
	std::vector<MeshComponent*> MeshComponents;
};

class Entity
{
public:
	Entity();

	virtual ~Entity();

	void AddComponment(Component* componment);

	void AddChild(Entity* child);

	void RemoveChild(Entity* child);

	void SetParent(Entity* parent);

	void AddToEmbreeScene(RTCDevice device_i, RTCScene scene_i);

	/*
	 * Get component with RTTI
	 * */
	template <typename Tp_>
	Tp_* GetComponent()
	{
		for (auto comp : Components_)
		{
			if (dynamic_cast<Tp_*>(comp))
			{
				return dynamic_cast<Tp_*>(comp);
			}
		}
		return nullptr;
	}

	template<>
	Transform* GetComponent<Transform>()
	{
		return &Transform_;
	}

	virtual void Tick(float dt);

	TVector3& GetPosition()
	{
		return Transform_.Position();
	}

	void SetPosition(const TVector3& position)
	{
		if (!IsTransformDirty_)
		{
			IsTransformDirty_ = Transform_.Position() == position;
		}
		Transform_.Position() = position;
	}

	void SetScale(const TVector3& scale)
	{
		if (!IsTransformDirty_)
		{
			IsTransformDirty_ = Transform_.Scale() == scale;
		}
		Transform_.Scale() = scale;
	}

	TVector3& GetScale()
	{
		return Transform_.Scale();
	}

	void SetRotation(const TVector3& rotation)
	{
		if (!IsTransformDirty_)
		{
			IsTransformDirty_ = Transform_.Rotation() == rotation;
		}
		Transform_.Rotation() = rotation;
	}

	TVector3& GetRotation()
	{
		return Transform_.Rotation();
	}

	const TMat4x4& GetWorldMatrix() const
	{
		UpdateWorldMatrix();

		return WorldMatrix_;
	}

	void UpdateWorldMatrix() const;

	void SetSelected(bool selected) { IsSelected_ = selected; }

	bool IsSelected() const { return IsSelected_; }

	void SetPhysicsData(void* data) { PhysicsData_ = data; }

protected:
	Entity*				  Parent_	   = nullptr;
	void*				  PhysicsData_ = nullptr;
	std::vector<Entity*>  Children_;
	std::list<Component*> Components_;
	mutable TMat4x4		  WorldMatrix_;
	Transform			  Transform_;
	// boolean fileds
	bool		 IsSelected_		 = false;
	mutable bool IsTransformDirty_	 = true;

	// Get Set Property
protected:
	GETSET_BOOL(CastShadow) = true;
	GETSET_BOOL(RecieveShadow) = true;
	GETSET_BOOL(IngoreSelfShadow) = true;
};
