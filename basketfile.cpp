#include "PrecompiledHeaders.h"

// still required from SKSE Common
#include "common/IFileStream.h"

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
	formList[WHITELIST] = dhnd->LookupForm<RE::BGSListForm>(g_whiteList_formid, MODNAME);
	formList[BLACKLIST] = dhnd->LookupForm<RE::BGSListForm>(g_blackList_formid, MODNAME);
}

inline UInt32 BasketFile::GetSize(listnum list_number)
{
	return formList[list_number]->forms.size();
}

bool BasketFile::SaveFile(listnum list_number, const char* basketText)
{
#if _DEBUG
	_MESSAGE("BasketFile::SaveFile");
#endif
	// NYI
	return false;

	std::string RuntimeDir = FileUtils::GetGamePath();
	if(RuntimeDir.empty())
		return false;

	std::string fullPath = RuntimeDir + "Data\\SKSE\\Plugins\\" + SHSE_NAME + "\\" + basketText;
	IFileStream fs;
	if(!fs.Create(fullPath.c_str()))
		return false;

	if(formList[list_number])
	{
		for (RE::TESForm* childForm : formList[list_number]->forms)
		{
			if (childForm)
			{
				std::string name = GetPluginName(GetFirstModInfo(childForm));
				UInt32 hexID = childForm->formID & 0x00FFFFFF;
				
				stringEx str;
				str.Format("%s\t0x%06x\t%s\n", name.c_str(), hexID, PluginUtils::GetBaseName(childForm).c_str());
				
				#if _DEBUG
				_MESSAGE("%s\t%08x\t%s", name.c_str(), hexID, PluginUtils::GetBaseName(childForm).c_str());
				#endif
				
				std::vector<char>buf(str.size() + 1);
				buf[str.size()] = 0;
				memcpy(&buf[0], str.c_str(),str.size());
				fs.WriteBuf(&buf[0], static_cast<UInt32>(str.size()));
			}
			
			if(formList[list_number]->scriptAddedTempForms)
			{
				for(RE::FormID formid : *formList[list_number]->scriptAddedTempForms)
				{
					RE::TESForm* childForm = RE::TESForm::LookupByID(formid);
					if(childForm)
					{
						std::string name = GetPluginName(GetFirstModInfo(childForm));
						UInt32 hexID = childForm->formID & 0x00FFFFFF;

						stringEx str;
						str.Format("%s\t0x%06x\n", name.c_str(), hexID);

						#if _DEBUG
						_MESSAGE("%s\t%08x", name.c_str(), hexID);
						#endif
						
						std::vector<char>buf(str.size() + 1);
						buf[str.size()] = 0;
						memcpy(&buf[0], str.c_str(), str.size());
						fs.WriteBuf(&buf[0], static_cast<UInt32>(str.length()));
					}
				}
			}
		}
	}
	fs.Close();

#if _DEBUG
	_MESSAGE("BasketFile::SaveFile end");
#endif
	return true;
}

bool BasketFile::LoadFile(listnum list_number, const char* basketText)
{
#if _DEBUG
	_MESSAGE("BasketFile::LoadFile");
#endif
	// NYI
	return false;

	std::string RuntimeDir = FileUtils::GetGamePath();
	if(RuntimeDir.empty())
		return false;

	std::string fullPath = RuntimeDir + "Data\\SKSE\\Plugins\\" + SHSE_NAME + "\\" + basketText;

	IFileStream fs;
	if(!fs.Open(fullPath.c_str()))
		return false;

	for (int index = 0; !fs.HitEOF(); index++)
	{
		char buf[512];
		fs.ReadString(buf, 512, '\n', '\r');

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
		
		formList[list_number]->AddForm(thisForm);
	}
	fs.Close();
#if _DEBUG
	_MESSAGE("BasketFile::LoadFile end");
#endif
	return true;
}

void BasketFile::SyncList(listnum list_number)
{
	if (formList[list_number])
	{
		// unblock forms which may have been blacklisted and are no longer
		if (list_number == BLACKLIST)
		{
			DataCase* data(DataCase::GetInstance());
			for (auto form : list[list_number])
			{
				data->UnblockForm(form);
			}
		}

		list[list_number].clear();

		for (RE::TESForm* listMember : formList[list_number]->forms)
		{
#if _DEBUG
			_MESSAGE("FormID 0x%08x stored on list %d", listMember->GetFormID(), list_number);
#endif
			list[list_number].insert(listMember);
		}
		if (formList[list_number]->scriptAddedTempForms)
		{
			for (RE::FormID formid : *formList[list_number]->scriptAddedTempForms)
			{
				RE::TESForm* childForm = RE::TESForm::LookupByID(formid);
				if (childForm)
				{
#if _DEBUG
					_MESSAGE("FormID 0x%08x (temp) stored on list %d", childForm->GetFormID(), list_number);
#endif
					list[list_number].insert(childForm);
				}
				else
				{
#if _DEBUG
					_MESSAGE("FormID 0x%08x (temp) not mapped to Form", formid);
#endif
				}
			}
		}
		// unblock forms which may have been checked and blocked prior to whitelisting
		if (list_number == WHITELIST)
		{
			DataCase* data(DataCase::GetInstance());
			for (auto form : list[list_number])
			{
				data->UnblockForm(form);
			}
		}
	}
}

const std::unordered_set<const RE::TESForm*> BasketFile::GetList(listnum list_number) const
{
	return list[list_number];
}

bool BasketFile::IsinList(listnum list_number, const RE::TESForm * pickupform) const
{
	return list[list_number].count(pickupform) > 0;
}

