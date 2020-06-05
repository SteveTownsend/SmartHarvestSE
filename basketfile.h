#pragma once

class BasketFile
{
public:
	enum listnum
	{
		WHITELIST = 0,
		BLACKLIST,
		MAX,
	};

	UInt32 GetSize(listnum list_number);
	bool IsinList(listnum list_number, const RE::TESForm* form) const;
	bool SaveFile(listnum list_number, const char* basketText);
	bool LoadFile(listnum list_number, const char* basketText);

	void SyncList(listnum list_number);

	static BasketFile* GetSingleton(void)
	{
		if (!s_pInstance)
			s_pInstance = new BasketFile();
		return s_pInstance;
	}

	const std::unordered_set<const RE::TESForm*> GetList(listnum list_number) const;
private:
	RE::BGSListForm* formList[MAX];
	std::unordered_set<const RE::TESForm*> list[MAX];
	static BasketFile* s_pInstance;

	BasketFile(void);
};

class stringEx : public std::string
{
public:
	stringEx() : std::string() {}
	stringEx(const char *cc) : std::string(cc) {}
	stringEx(const std::string &str) : std::string(str) {}
	stringEx(const stringEx &str) : std::string(str) {}
	stringEx& operator=(const char *cc) { std::string::operator=(cc); return *this; }
	stringEx& operator=(const std::string& str) { std::string::operator=(str); return *this; }
	
	stringEx Format(const char* fmt, ...)
	{
		va_list list; va_start(list, fmt);
		char table[5000]; {
			vsprintf_s(table, 5000, fmt, list);
			assign(table);
		}
		va_end(list);
		return *this;
	}

	stringEx& MakeUpper() {
		std::transform(cbegin(), cend(), begin(), [&] (char ch) -> char { return toupper(ch); });
		return *this;
	}
};
