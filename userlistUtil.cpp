#include "skse64/GameRTTI.h"
#include "skse64/GameData.h"
#include <common/IFileStream.h>
#include <vector>
#include <string>
#include <sstream>
//#include <algorithm> //Trim
#include <iomanip>

#include "dataCase.h"
#include "userlistUtil.h"
#include "utils.h"

#include <fstream>
int UserlistSave()
{
	static std::string settingsFile;
	if (settingsFile.empty())
	{
		std::string settingsFile = FileUtils::GetPluginPath();
		if (settingsFile.empty())
			return -1;
		settingsFile += USERLIST_FILE;

#ifdef _DEBUG
		_MESSAGE("%s", settingsFile.c_str());
#endif
	}

	int result = 0;

	//std::ifstream ifs;
	//ifs.open()



	IFileStream fs;
	if (!fs.Create(settingsFile.c_str()))
		return -2;

	for (auto pForm : DataCase::GetInstance()->lists.userlist)
	{
		if (!pForm)
			continue;

		std::string modName = PluginUtils::GetPluginName(pForm);
		if (modName.empty())
			continue;

		UInt32 baseID = pForm->formID & 0x00FFFFFF;
		if (baseID == 0x0)
			continue;

		std::string str = modName.c_str();
		str += '\t';
		str += ToStringID(baseID);
		str += '\n';
		fs.WriteBuf(str.c_str(), str.size());
		result += 1;
	}

	fs.Close();
	return result;
}

int UserlistLoad()
{
	static BGSListForm* formlist;

	if (!formlist)
	{
		UInt8 modIndex = PluginUtils::GetLoadedModIndex("AutoHarvestSE.esp");
		if (modIndex == 255)
			return -1;
		UInt32 formID = 0x0333c;
		formID |= (modIndex << 24);
		formlist = static_cast<BGSListForm*>(LookupFormByID(formID));

		if (!formlist)
			return -1;
	}

	std::string settingsFile = FileUtils::GetPluginPath();
	if (settingsFile.empty())
		return -1;

	settingsFile += USERLIST_FILE;

#ifdef _DEBUG
	_MESSAGE("%s", settingsFile.c_str());
#endif

	IFileStream fs;
	if (!fs.Open(settingsFile.c_str()))
		return -2;

	std::vector<TESForm*> list;

	for (int index = 0; !fs.HitEOF(); ++index)
	{
		char buf[512];
		fs.ReadString(buf, 512, '\n', '\r');

		if (buf[0] == '#')
			continue;

		std::string bufLine = buf;

		if (bufLine.empty())
			continue;

		TESForm* pForm = GetLineForm(bufLine, '\t');
		if (pForm)
			list.push_back(pForm);
	}
	fs.Close();

	if (list.size() == 0)
		return 0;

	int result = 0;
	DataCase* data = DataCase::GetInstance();

	data->lists.userlist.clear();
	CALL_MEMBER_FN(formlist, RevertList)();

	for (auto item : list)
	{
		data->lists.userlist.insert(item);
		CALL_MEMBER_FN(formlist, AddFormToList)(item);
		result += 1;
	}

	return result;
}

UInt32 atoul(const char* chr)
{
	UInt32 result = 0;
	try {
		result = std::stoul(chr, nullptr, 16);
	}
	catch (std::invalid_argument err) {
		result = 0;
	}
	catch (std::out_of_range err) {
		result = 0;
	}
	return result;
}

inline std::string ToStringID(UInt32 id)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::setw(8) << std::uppercase << id;
	return ss.str();
}

std::vector<std::string> Split(const std::string &str, char delim)
{
	std::vector<std::string> res;
	size_t current = 0, found;
	while ((found = str.find_first_of(delim, current)) != std::string::npos){
		res.push_back(std::string(str, current, found - current));
		current = found + 1;
	}
	res.push_back(std::string(str, current, str.size() - current));
	return res;
}

TESForm* GetLineForm(const std::string &str, char delim)
{
	std::vector<std::string> sp = Split(str, delim);
	if (sp.size() != 2)
		return nullptr;

	std::string fileName = sp.at(0);
	if (fileName.empty())
		return nullptr;

	StringUtils::ToUpper(fileName);
	if ((fileName.find(".ESM", 0) == std::string::npos) && (fileName.find(".ESP", 0) == std::string::npos) && (fileName.find(".ESL", 0) == std::string::npos))
		return nullptr;

	UInt32 modIndex = PluginUtils::GetLoadedModIndex(fileName.c_str());
	if (modIndex == 0xFF)
		return nullptr;

	TESForm* pForm = nullptr;
	std::string ID = sp.at(1);
	if (!ID.empty())
	{
		UInt32 formID = atoul(ID.c_str());
		formID |= (modIndex << 24);
		pForm = LookupFormByID(formID);
	}

	if (!pForm)
		return nullptr;

	return pForm;
}
