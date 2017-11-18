#pragma once
#include "pch.h"

//---------------------------------------------------------------------------------------------------------------------
// EvtData_New_Actor - This event is sent out when an actor is *actually* created.
//---------------------------------------------------------------------------------------------------------------------
class EvtData_New_Actor : public BaseEventData
{
	ActorId m_actorId;

public:
	static const EventType sk_EventType;

	EvtData_New_Actor(void)
	{
		m_actorId = 0;
	}

	explicit EvtData_New_Actor(ActorId actorId)
		: m_actorId(actorId)
		
	{
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_actorId;
	}

	virtual const EventType& VGetEventType(void) const
	{
		return sk_EventType;
	}

	virtual IEvent* VCopy(void) const
	{
		return new EvtData_New_Actor(m_actorId);
	}

	virtual void VSerialize(std::ostrstream& out) const
	{
		out << m_actorId << " ";
	}


	virtual const char* GetName(void) const
	{
		return "EvtData_New_Actor";
	}

	const ActorId GetActorId(void) const
	{
		return m_actorId;
	}

	
};


//---------------------------------------------------------------------------------------------------------------------
// EvtData_Destroy_Actor - sent when actors are destroyed	
//---------------------------------------------------------------------------------------------------------------------
class EvtData_Destroy_Actor : public BaseEventData
{
	ActorId m_id;

public:
	static const EventType sk_EventType;

	explicit EvtData_Destroy_Actor(ActorId id = 0)
		: m_id(id)
	{
		//
	}

	virtual const EventType& VGetEventType(void) const
	{
		return sk_EventType;
	}

	virtual IEvent* VCopy(void) const
	{
		return new EvtData_Destroy_Actor(m_id);
	}

	virtual void VSerialize(std::ostrstream &out) const
	{
		out << m_id;
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_id;
	}

	virtual const char* GetName(void) const
	{
		return "EvtData_Destroy_Actor";
	}

	ActorId GetId(void) const { return m_id; }
};

//---------------------------------------------------------------------------------------------------------------------
// EvtData_Move_Actor - sent when actors are moved
//---------------------------------------------------------------------------------------------------------------------
class EvtData_Move_Actor : public BaseEventData
{
	ActorId m_id;
	mat4 m_matrix;

public:
	static const EventType sk_EventType;

	virtual const EventType& VGetEventType(void) const
	{
		return sk_EventType;
	}

	EvtData_Move_Actor(void)
	{
		m_id = 0;
	}

	EvtData_Move_Actor(ActorId id, const mat4& matrix)
		: m_id(id), m_matrix(matrix)
	{
		//
	}

	virtual void VSerialize(std::ostrstream &out) const
	{
		out << m_id << " ";
		//out << m_matrix;
	}

	virtual void VDeserialize(std::istrstream& in)
	{
		in >> m_id;
		//in >> m_matrix;
	}

	virtual IEvent* VCopy() const
	{
		return new EvtData_Move_Actor(m_id, m_matrix);
	}

	virtual const char* GetName(void) const
	{
		return "EvtData_Move_Actor";
	}

	ActorId GetId(void) const
	{
		return m_id;
	}

	const mat4& GetMatrix(void) const
	{
		return m_matrix;
	}
};