/*** LICENCE ***************************************************************************************/
/*
  SimpleIni - Simple class for configuration file like .ini

  This file is part of SimpleIni.

	SimpleIni is free software : you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SimpleIni is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SimpleIni.  If not, see <http://www.gnu.org/licenses/>.
*/
/***************************************************************************************************/
#include "PrecompiledHeaders.h"
#include <iostream>
#include <stdexcept>
#include "SimpleIni.h"

using namespace std;

/**************************************************************************************************************/
/***                                                                                                        ***/
/*** Class SimpleIni                                                                                        ***/
/***                                                                                                        ***/
/**************************************************************************************************************/
SimpleIni::SimpleIni(const string& filename) : m_OptionCommentCharacters(";#")
{
	if(filename!="")
	{
		if(!Load(filename)) throw logic_error("Unable to open the file "+filename+" in read mode.");
	}
}

SimpleIni::~SimpleIni()
{
	Free();
}

void SimpleIni::SetOptions(optionKey key, const std::string& value)
{
	switch(key)
	{
		case optionKey::Comment :
			m_OptionCommentCharacters = value;
	}
}

bool SimpleIni::Load(const string& filename)
{
	size_t pos;
	size_t pos2;
	size_t length;
	string line;
	string section;
	string key;
	string comment;
	ifstream file;
	IniLine iniLine;


	Free();
	m_FileName = filename;

	//*** Ouverture du fichier
	file.open(m_FileName.c_str(), ifstream::in);
	if(!file) return false;

	//*** Parcours du fichier
	while(getline(file, line))
	{
		ParasitCar(line);
		if(line.empty()) continue;
		length = line.length();

		//*** Raz
		key = "";
		iniLine.value = "";
		iniLine.comment = "";

		//*** Section ?
		if(line.at(0)=='[')
		{
			pos = line.find_first_of(']');
			if(pos==string::npos) pos = line.length();
			section = Trim(line.substr(1, pos-1));
			if(comment!="")
			{
				m_DescriptionMap[section][""] = comment;
				comment = "";
			}
			continue;
		}

		//*** Commentaire ?
		pos=string::npos;
		for(unsigned int i = 0; i < m_OptionCommentCharacters.length(); ++i)
		{
			pos2 = line.find_first_of(m_OptionCommentCharacters[i]);
			if(pos2==string::npos) continue;
			if(pos==string::npos)
			{
				pos=pos2;
				continue;
			}
			if(pos>pos2) pos = pos2;
		}
		if(pos!=string::npos)
		{
			if(pos>0)
			{
				iniLine.comment = line.substr(pos+1, length-pos);
				line.erase(pos, length-pos);
			}
			else
			{
				if(comment!="") comment += '\n';
				comment += line.substr(pos+1, length-pos);
				continue;
			}
		}

		//*** Valeur ?
		pos = line.find_first_of('=');
		if(pos!=string::npos)
		{
			iniLine.value = Trim(line.substr(pos+1, length-pos));
			line.erase(pos, length-pos);
		}

		//*** Mémorisation
		key = Trim(line);
		m_IniMap[section][key] = iniLine;
		if(comment!="")
		{
			m_DescriptionMap[section][key] = comment;
			comment = "";
		}

	}

	file.close();
	return true;
}

bool SimpleIni::Save()
{
	return SaveAs(m_FileName);
}

bool SimpleIni::SaveAs(const string& filename)
{
	std::map<std::string, std::map<std::string, SimpleIni::IniLine> >::iterator itSection;
	std::map<std::string, SimpleIni::IniLine>::iterator itKey;
	IniLine iniLine;
	ofstream file;
	bool first = true;

	file.open(filename.c_str());
	if(!file) return false;

	for(itSection=m_IniMap.begin(); itSection!=m_IniMap.end(); ++itSection)
	{
		if(!first) file << endl;
		SaveDescription(itSection->first, "", file);
		if(itSection->first!="") file << "[" << itSection->first << "]" << endl;

		for(itKey=itSection->second.begin(); itKey!=itSection->second.end(); ++itKey)
		{
			SaveDescription(itSection->first, itKey->first, file);
			iniLine = itKey->second;
			if(itKey->first != "") file << itKey->first << "=" << iniLine.value;
			if(iniLine.comment != "")
			{
				if(itKey->first != "")
					file << "\t;";
				else
					file << "#";
				file << iniLine.comment;
			}
			file << endl;
		}
		first = false;
	}

	file.close();

	return true;
}

void SimpleIni::SaveDescription(string section, string key, ofstream &file)
{
	stringstream ss(m_DescriptionMap[section][key]);
	string item;
	while (std::getline(ss, item, '\n'))
	{
		file << "#" << item << endl;
	}
}

void SimpleIni::Free()
{
	m_IniMap.clear();
}

string SimpleIni::GetValue(const string& section, const string& key, const string& defaultValue)
{
	map<string, map<string, SimpleIni::IniLine> >::iterator itSection=m_IniMap.find(section);
	if(itSection == m_IniMap.end()) return defaultValue;

	map<string, SimpleIni::IniLine>::iterator itKey=itSection->second.find(key);
	if(itKey == itSection->second.end()) return defaultValue;

	return itKey->second.value;
}

void SimpleIni::SetValue(const string& section, const string& key, const string& value)
{
	IniLine iniLine;

	iniLine = m_IniMap[section][key];
	iniLine.value = value;
	m_IniMap[section][key] = iniLine;
}

string SimpleIni::GetComment(const string& section, const string& key)
{
	map<string, map<string, SimpleIni::IniLine> >::iterator itSection=m_IniMap.find(section);
	if(itSection == m_IniMap.end()) return "";

	map<string, SimpleIni::IniLine>::iterator itKey=itSection->second.find(key);
	if(itKey == itSection->second.end()) return "";

	return itKey->second.comment;
}

void SimpleIni::SetComment(const string& section, const string& key, const string& comment)
{
	IniLine iniLine;

	iniLine = m_IniMap[section][key];
	iniLine.comment = comment;
	m_IniMap[section][key] = iniLine;
}

void SimpleIni::DeleteKey(const string& section, const string& key)
{
	m_IniMap[section].erase(key);
}

SimpleIni::SectionIterator SimpleIni::beginSection()
{
	return SectionIterator(m_IniMap.begin());
}

SimpleIni::SectionIterator SimpleIni::endSection()
{
	return SectionIterator(m_IniMap.end());
}

SimpleIni::KeyIterator SimpleIni::beginKey(const std::string& section)
{
	map<string, map<string, SimpleIni::IniLine> >::iterator itSection=m_IniMap.find(section);
	if(itSection == m_IniMap.end())
	{
		itSection = m_IniMap.begin();
		return KeyIterator(itSection->second.end());
	}

	return KeyIterator(itSection->second.begin());
}

SimpleIni::KeyIterator SimpleIni::endKey(const std::string& section)
{
	map<string, map<string, SimpleIni::IniLine> >::iterator itSection=m_IniMap.find(section);
	if(itSection == m_IniMap.end()) itSection = m_IniMap.begin();

	return KeyIterator(itSection->second.end());
}

void SimpleIni::ParasitCar(string& str)
{
	size_t fin=str.size();

	if(fin<1) return;

	if(str.at(fin-1)<' ') str.erase(fin-1);
}

string SimpleIni::Trim(const string& str)
{
	size_t deb=0;
	size_t fin=str.size();
	char   chr;

	while(deb<fin)
	{
		chr = str.at(deb);
		if( (chr!=' ') && (chr!='\t') ) break;
		deb++;
	}

	while(fin>0)
	{
		chr = str.at(fin-1);
		if( (chr!=' ') && (chr!='\t') ) break;
		fin--;
	}

	return str.substr(deb, fin-deb);
}

/**************************************************************************************************************/
/***                                                                                                        ***/
/*** Class SectionIterator                                                                                  ***/
/***                                                                                                        ***/
/**************************************************************************************************************/
SimpleIni::SectionIterator::SectionIterator()
{
}

SimpleIni::SectionIterator::SectionIterator(std::map<std::string, std::map<std::string, SimpleIni::IniLine> >::iterator mapIterator)
{
	m_mapIterator = mapIterator;
}

const std::string& SimpleIni::SectionIterator::operator*()
{
	return m_mapIterator->first;
}

SimpleIni::SectionIterator SimpleIni::SectionIterator::operator++()
{
	++m_mapIterator;
	return *this;
}

bool SimpleIni::SectionIterator::operator==(SectionIterator const& a)
{
	return a.m_mapIterator==m_mapIterator;
}

bool SimpleIni::SectionIterator::operator!=(SectionIterator const& a)
{
	return a.m_mapIterator!=m_mapIterator;
}

/**************************************************************************************************************/
/***                                                                                                        ***/
/*** Class KeyIterator                                                                                      ***/
/***                                                                                                        ***/
/**************************************************************************************************************/
SimpleIni::KeyIterator::KeyIterator()
{
}

SimpleIni::KeyIterator::KeyIterator(std::map<std::string, SimpleIni::IniLine>::iterator mapIterator)
{
	m_mapIterator = mapIterator;
}

const std::string& SimpleIni::KeyIterator::operator*()
{
	return m_mapIterator->first;
}

SimpleIni::KeyIterator SimpleIni::KeyIterator::operator++()
{
	++m_mapIterator;
	return *this;
}

bool SimpleIni::KeyIterator::operator==(KeyIterator const& a)
{
	return a.m_mapIterator==m_mapIterator;
}

bool SimpleIni::KeyIterator::operator!=(KeyIterator const& a)
{
	return a.m_mapIterator!=m_mapIterator;
}
