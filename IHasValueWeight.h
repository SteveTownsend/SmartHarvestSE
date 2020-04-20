#pragma once

class INIFile;

class IHasValueWeight
{
public:
	ObjectType GetObjectType() const;
	std::string GetTypeName() const;
	bool ValueWeightTooLowToLoot(INIFile* settings) const;

	virtual double GetWeight(void) const = 0;
	virtual double GetWorth(void) const = 0;

	static constexpr float ValueWeightMaximum = 1000.0f;

protected:
	virtual ~IHasValueWeight() {};
	virtual const char * GetName() const = 0;
	virtual UInt32 GetFormID() const = 0;

	std::string m_typeName;
	ObjectType m_objectType;
};