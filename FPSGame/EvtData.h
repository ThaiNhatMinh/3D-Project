

class EvtData_RequestCreateWeapon :public IEvent
{
public:
	EvtData_RequestCreateWeapon(Actor* p,const char* n) :Parent(p), WPName(n)
	{
	};
	static const EventType sk_EventType;
	virtual const EventType& VGetEventType(void) const
	{
		return sk_EventType;
	};
	virtual void VSerialize(std::ostrstream& out) const {};
	virtual void VDeserialize(std::istrstream& in) {};
	virtual IEvent* VCopy(void) const
	{
		return new EvtData_RequestCreateWeapon(Parent, WPName);
	};
	virtual const char* GetName(void) const
	{
		return "EvtData_RequestCreateActor";
	};
	std::string File;
	const char* WPName;
	Actor* Parent;
};

class EvtTakeDamage : public IEvent
{
public:
	EvtTakeDamage(Creature* attacker, Creature* victim, int damage);
	const Creature* GetAttacker();
	const Creature* GetVictim();
	int GetDamage();
public:
	EVENT_DEFINE(EvtTakeDamage)
private:
	Creature* Attacker;
	Creature* Victim;
	int Damage;
};

class EvtHPChange : public IEvent
{
public:
	EvtHPChange(Creature* target, int oldHP);
	const Creature* GetCreature();
	int GetOldHP();
public:
	EVENT_DEFINE(EvtHPChange)
private:
	Creature* Obj;
	int OldHP;
};

// Attack to all player 
class EvtExplosion : public IEvent
{
public:
	EvtExplosion(const vec3& pos,int damage,float range1, float range2);
	
	int		GetDamage();
	float	GetRange1();
	float	GetRange2();
	const	vec3& GetPos();
public:
	EVENT_DEFINE(EvtTakeDamage)
private:
	vec3	Pos;
	float	Range1;
	float	Range2;
	int		Damage;
};