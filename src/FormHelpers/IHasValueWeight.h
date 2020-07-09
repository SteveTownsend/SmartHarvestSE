#pragma once

class INIFile;

class IHasValueWeight
{
public:
	ObjectType GetObjectType() const;
	std::string GetTypeName() const;
	bool ValueWeightTooLowToLoot() const;
	bool IsValuable() const;

	virtual double GetWeight(void) const = 0;
	SInt32 GetWorth(void) const;

	static constexpr float ValueWeightMaximum = 1000.0f;

protected:
	IHasValueWeight() : m_objectType(ObjectType::unknown), m_worth(0), m_worthSetup(false) {}
	virtual ~IHasValueWeight() {}

	virtual const char * GetName() const = 0;
	virtual UInt32 GetFormID() const = 0;
	virtual SInt32 CalculateWorth(void) const = 0;

	std::string m_typeName;
	ObjectType m_objectType;
	mutable SInt32 m_worth;
	mutable bool m_worthSetup;
};