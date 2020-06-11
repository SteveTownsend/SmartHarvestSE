#include "PrecompiledHeaders.h"

#include "utils.h"
#include "basketfile.h"

BasketFile* BasketFile::s_pInstance = nullptr;

UInt32 g_whiteList_formid = 0x333c;
UInt32 g_blackList_formid = 0x0333d;

namespace
{
	const RE::TESFile* GetFirstModInfo(RE::TESForm* thisForm)
	{
		UInt16 modIndex = (thisForm->formID) >> 24;
		if (modIndex == 0xFF)
			return nullptr;
		
		RE::TESDataHandler* dhnd(RE::TESDataHandler::GetSingleton());
		if (!dhnd)
			return nullptr;

		if (modIndex < 0xFE)
		{
			return dhnd->LookupLoadedModByIndex(static_cast<UInt8>(modIndex));
		}
		else if (modIndex == 0xFE)
		{
			return dhnd->LookupLoadedLightModByIndex(modIndex);
		}

		return nullptr;
	}

	std::string GetPluginName(const RE::TESFile* modinfo)
	{
		return modinfo ? modinfo->fileName : std::string();
	}

	std::vector<std::string> split(const std::string &str, char delim)
	{
		std::vector<std::string> res;
		size_t current = 0, found;
		while((found = str.find_first_of(delim, current)) != std::string::npos){
		res.push_back(std::string(str, current, found - current));
		current = found + 1;
		}
		res.push_back(std::string(str, current, str.size() - current));
		return res;
	}
}

BasketFile::BasketFile()
{
	RE::TESDataHandler* dhnd(RE::TESDataHandler::GetSingleton());
	formList[int(listnum::WHITELIST)] = dhnd->LookupForm<RE::BGSListForm>(g_whiteList_formid, MODNAME);
	formList[int(listnum::BLACKLIST)] = dhnd->LookupForm<RE::BGSListForm>(g_blackList_formid, MODNAME);
}

inline UInt32 BasketFile::GetSize(listnum list_number)
{
	return formList[int(list_number)]->forms.size();
}

bool BasketFile::SaveFile(listnum list_number, const char* basketText)
{
	DBG_VMESSAGE("BasketFile::SaveFile");
	// NYI
	return false;

	std::string RuntimeDir = FileUtils::GetGamePath();
	if(RuntimeDir.empty())
		return false;

	std::string fullPath = RuntimeDir + "Data\\SKSE\\Plugins\\" + SHSE_NAME + "\\" + basketText;
	std::ofstream fs(fullPath);
	if (fs.fail())
	{
		REL_ERROR("Basket File %s cannot be opened for output", fullPath.c_str());
		return false;
	}

	if(formList[int(list_number)])
	{
		for (RE::TESForm* childForm : formList[int(list_number)]->forms)
		{
			if (childForm)
			{
				std::string name = GetPluginName(GetFirstModInfo(childForm));
				UInt32 hexID = childForm->formID & 0x00FFFFFF;
				
				stringEx str;
				str.Format("%s\t0x%06x\t%s\n", name.c_str(), hexID, PluginUtils::GetBaseName(childForm).c_str());
				DBG_VMESSAGE("%s\t%08x\t%s", name.c_str(), hexID, PluginUtils::GetBaseName(childForm).c_str());
				
				std::vector<char>buf(str.size() + 1);
				buf[str.size()] = 0;
				memcpy(&buf[0], str.c_str(),str.size());
				fs.write(&buf[0], str.size());
			}
			
			if(formList[int(list_number)]->scriptAddedTempForms)
			{
				for(RE::FormID formid : *formList[int(list_number)]->scriptAddedTempForms)
				{
					RE::TESForm* childForm = RE::TESForm::LookupByID(formid);
					if(childForm)
					{
						std::string name = GetPluginName(GetFirstModInfo(childForm));
						UInt32 hexID = childForm->formID & 0x00FFFFFF;

						stringEx str;
						str.Format("%s\t0x%06x\n", name.c_str(), hexID);
						DBG_VMESSAGE("%s\t%08x", name.c_str(), hexID);
						
						std::vector<char>buf(str.size() + 1);
						buf[str.size()] = 0;
						memcpy(&buf[0], str.c_str(), str.size());
						fs.write(&buf[0], str.length());
					}
				}
			}
		}
	}
	fs.close();

	DBG_VMESSAGE("BasketFile::SaveFile end");
	return true;
}

bool BasketFile::LoadFile(listnum list_number, const char* basketText)
{
	DBG_VMESSAGE("BasketFile::LoadFile");
	// NYI
	return false;

	std::string RuntimeDir = FileUtils::GetGamePath();
	if(RuntimeDir.empty())
		return false;

	std::string fullPath = RuntimeDir + "Data\\SKSE\\Plugins\\" + SHSE_NAME + "\\" + basketText;

	std::ifstream fs(fullPath);
	if (fs.fail())
	{
		REL_ERROR("Basket File %s cannot be opened for output", fullPath.c_str());
		return false;
	}

	char buf[512];
	for (int index = 0; !fs.eof() && !fs.fail(); ++index)
	{
		fs.getline(buf, 512);

		// skip comments
		if (buf[0] == '#')
			continue;

		std::vector<std::string> str;
		std::string bufLine = buf;
		str = split(bufLine, '\t');

		//if (str.size() != 2 && str.size() != 3)
		//	continue;
		//if (str[0].empty() || str[1].empty())
		//	continue;

		const RE::TESFile* modinfo = RE::TESDataHandler::GetSingleton()->LookupModByName(str[0].c_str());
		if (!modinfo)
			continue;

		// modIndex logic from SKSE64's GameData.h -> GetPartialIndex()
		UInt32 modIndex(0);
		if ((modinfo->recordFlags & RE::TESFile::RecordFlag::kSmallFile) == RE::TESFile::RecordFlag::kSmallFile)
		{
			modIndex = (0xFE000 | modinfo->smallFileCompileIndex);
		}
		else
		{
			modIndex = modinfo->compileIndex;
		}

		UInt32 formID = std::stoul(str[1], nullptr, 16);
		formID |= (modIndex << 24);
		
		RE::TESForm* thisForm = (RE::TESForm*)RE::TESForm::LookupByID(formID);
		if (!thisForm)
			continue;
		
		formList[int(list_number)]->AddForm(thisForm);
	}
	fs.close();
	DBG_VMESSAGE("BasketFile::LoadFile end");
	return true;
}

void BasketFile::SyncList(listnum list_number)
{
	if (formList[int(list_number)])
	{
		// unblock forms which may have been blacklisted and are no longer
		if (list_number == listnum::BLACKLIST)
		{
			DataCase* data(DataCase::GetInstance());
			for (auto form : list[int(list_number)])
			{
				data->UnblockForm(form);
			}
		}

		list[int(list_number)].clear();

		for (RE::TESForm* listMember : formList[int(list_number)]->forms)
		{
			DBG_VMESSAGE("FormID 0x%08x stored on list %d", listMember->GetFormID(), list_number);
			list[int(list_number)].insert(listMember);
		}
		if (formList[int(list_number)]->scriptAddedTempForms)
		{
			for (RE::FormID formid : *formList[int(list_number)]->scriptAddedTempForms)
			{
				RE::TESForm* childForm = RE::TESForm::LookupByID(formid);
				if (childForm)
				{
					DBG_VMESSAGE("FormID 0x%08x (temp) stored on list %d", childForm->GetFormID(), list_number);
					list[int(list_number)].insert(childForm);
				}
				else
				{
					DBG_VMESSAGE("FormID 0x%08x (temp) not mapped to Form", formid);
				}
			}
		}
		// unblock forms which may have been checked and blocked prior to whitelisting
		if (list_number == listnum::WHITELIST)
		{
			DataCase* data(DataCase::GetInstance());
			for (auto form : list[int(list_number)])
			{
				data->UnblockForm(form);
			}
		}
	}
}

const std::unordered_set<const RE::TESForm*> BasketFile::GetList(listnum list_number) const
{
	return list[int(list_number)];
}

bool BasketFile::IsinList(listnum list_number, const RE::TESForm * pickupform) const
{
	return list[int(list_number)].count(pickupform) > 0;
}

