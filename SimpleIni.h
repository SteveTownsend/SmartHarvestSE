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

/*** MAIN PAGE FOR DOXYGEN *************************************************************************/
/// \mainpage SimpleIni Class Documentation
/// \section intro_sec Introduction
///
/// This class allows you to easily manage configuration files like .ini on Windows or .conf on Linux, with less than 10 methods.\n
/// To use, include in your project SimpleIni.cpp and SimpleIni.h.
///
/// \section feature_sec Features
///
/// \li Get and Set key's values
/// \li Get and Set comments
/// \li Browse sections and keys
/// \li Comments identifed by ; or #
/// \li Removal of whitespace around sections, keys and values.
/// \li Keys defined before any section are placed in a section with blank name
/// \li Don't support multi-line values.
/// \li Comments on the section line are lost during the save method.
/// \li Compile on Linux and Windows, Intel or ARM.
///
/// \section portability_sec Portability
/// Unit tests passed successfully on :
/// \li Windows Seven (CPU Intel Celeron)
/// \li Linux Ubuntu (CPU Intel Atom)
/// \li Linux Raspian on Raspberry Pi (CPU ARM)
/// \li Linux FunPlug on NAS DNS-320 (CPU ARM)\n
/// (Compilation directives define LINUX or WIN only necessary for colours in unit tests)
///
/// \section example_sec Example
/// \code
/// #include <iostream>
/// #include "SimpleIni.h"
///
/// using namespace std;
///
/// int main()
/// {
///     SimpleIni ini;
///
///     ini.Load("examples\\example1.ini");
///     cout << "SGBD Host : " << ini.GetValue<string>("SGBD", "Host", "127.0.0.1") << endl;
///     cout << "Port TCP : " << ini.GetValue<int>("SGBD", "PortTCP", 3306) << endl;
///     cout << "Database : " << ini.GetValue<string>("SGBD", "BDD", "MyDB") << endl;
///
///     return 0;
/// }
/// \endcode
///
/// \section FileFormat_sec File format
/// INI files are parsed line by line, each line may be one of the following :
/// \li A section as [section-name]
/// \li A property as key = value
/// \li A comment, begin by # or ;
/// \li A blank line.
/// \n
/// File example :
/// \code
/// Key01 = Value01
/// Key02 = Value02
///
/// [Section1]
/// Key11 = Value11
/// Key12 = Value12
///
/// [SGBD]
/// #Host by name or IP Adresse
/// Host = 127.0.0.1
/// #TCP Port of the DB Server
/// PortTCP = 3306
/// #Database
/// BDD = MyDB  ;Use MyDBdebug for debug
/// \endcode
///
/// \section WhatsNew1_sec What's New in version 1.0
/// \li Parameters optimisation with const & when is possible
/// \li New unit test : Comparison of a configuration file for Windows with the same for Linux
/// \li Improvement of iterators, better seal with the inner workings of the class
/// \li Use prefixe m_ on privates members
///
/// \section licence_sec Licence
///  SimpleIni is free software : you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n\n
///  SimpleIni is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\n
///  You should have received a copy of the GNU General Public License along with SimpleIni. If not, see <http://www.gnu.org/licenses/>.
///
/***************************************************************************************************/

#ifndef SIMPLEINI_H
#define SIMPLEINI_H

#include <string>
#include <fstream>
#include <sstream>
#include <map>

/// \brief    Very simple class to manage configuration files
/// \details  Class allows you to easily manage configuration files with less than 10 methods.
class SimpleIni
{
	private:
		struct IniLine
		{
			std::string value;
			std::string comment;
		};

	public:
		enum class optionKey {Comment};

		/// \brief    Iterator for sections
		/// \details  Iterator for sections return a string reference on section's name.
		class SectionIterator;

		/// \brief    Iterator for keys
		/// \details  Iterator for keys on a section return a string reference on key's name.
		//ITER
		//typedef std::map<std::string, SimpleIni::IniLine>::iterator KeyIterator;
		class KeyIterator;

		/// \brief    Constructor of SimpleIni
		/// \param    filename         Name of the configuration file.
		/// \details  Constructor of SimpleIni, optionally can load configuration file \a filename, by Load method. If the Load method fails, an exception is raised.
		SimpleIni(const std::string& filename="");

		/// \brief    Destructor of SimpleIni
		/// \details  Destructor of SimpleIni, deallocate memory, like Free method.
		~SimpleIni();

		/// \brief    Deallocate memory
		/// \details  Deallocate memory stored by the last configuration file.
		void Free();

		/// \brief    Read a configuration file
		/// \details  Load a configuration file and store in memory, previous memory are deallocated.
		/// \param    filename         Name of the configuration file.
		/// \return   True if reading was successful, false otherwise.
		bool Load(const std::string& filename);

		/// \brief    Write the configuration file
		/// \details  Save the configuration on the disk.
		/// \return   True if writing was successful, false otherwise.
		bool Save();

		/// \brief    Write the configuration file as an other name
		/// \details  Save the configuration file on the disk as an other name.
		/// \param    filename         Name of the configuration file.
		/// \return   True if writing was successful, false otherwise.
		bool SaveAs(const std::string& filename);

		/// \brief    Get a string value
		/// \details  Get the value as string for a pair section/key.
		/// \param    section       Section to search
		/// \param    key           Key to search
		/// \param    defaultValue  Value returned if pair section/key not found
		/// \return   The value if it's found, \a defaultValue otherwise.
		std::string GetValue(const std::string& section, const std::string& key, const std::string& defaultValue);

		/// \brief    Get a generic value
		/// \details  Get the value generic for a pair section/key.
		/// \param    section       Section to search
		/// \param    key           Key to search
		/// \param    defaultValue  Value returned if pair section/key not found
		/// \return   The value if it's found, \a defaultValue otherwise.
		template <class T> T GetValue(const std::string& section, const std::string& key, const T& defaultValue)
		{
			std::string def = ";";
			std::string value = GetValue(section, key, def);
			if(value==def) return defaultValue;

			std::istringstream iss(value);
			T val;
			iss >> val;
			return val;
		}

		/// \brief    Set a string value
		/// \details  Set the value as string for a pair section/key.
		/// \param    section       Section to add or modify
		/// \param    key           Key to add or modify
		/// \param    value         Value to set
		void SetValue(const std::string& section, const std::string& key, const std::string& value);

		/// \brief    Set a generic value
		/// \details  Set the value generic for a pair section/key.
		/// \param    section       Section to add or modify
		/// \param    key           Key to add or modify
		/// \param    value         Value to set
		template <class T> void SetValue(const std::string& section, const std::string& key, const T& value)
		{
			std::ostringstream oss;
			std::string str;

			oss << value;
			str = oss.str();
			SetValue(section, key, str);
		}

		/// \brief    Get a comment
		/// \details  Get the comment for a pair section/key.
		/// \param    section       Section to search
		/// \param    key           Key to search
		/// \return   The comment if it's found, "" otherwise.
		std::string GetComment(const std::string& section, const std::string& key);

		/// \brief    Set a comment
		/// \details  Set the comment for a pair section/key.
		/// \param    section       Section to add or modify
		/// \param    key           Key to add or modify
		/// \param    comment       Comment to set
		void SetComment(const std::string& section, const std::string& key, const std::string& comment);

		/// \brief    Remove a key
		/// \details  Delete a key with value and comment.
		/// \param    section       Section to delete
		/// \param    key           Key to delete
		void DeleteKey(const std::string& section, const std::string& key);

		/// \brief    Return the first section iterator
		/// \details  Return an iterator that designates the first section
		/// \return   Iterator on the first section
		SectionIterator beginSection();

		/// \brief    Return the end section iterator
		/// \details  Return an iterator just beyond the last section
		/// \return   Iterator just beyond the last section
		SectionIterator endSection();

		/// \brief    Return the first key iterator
		/// \details  Return an iterator that designates the first key in the section
		/// \param    section       Section to browse
		/// \return   Iterator on the first key in the section
		KeyIterator beginKey(const std::string& section);

		/// \brief    Return the end key iterator
		/// \details  Return an iterator just beyond the last key in the section
		/// \param    section       Section to browse
		/// \return   Iterator just beyond the last key in the section
		KeyIterator endKey(const std::string& section);

		void SetOptions(optionKey key, const std::string& value);

	private:
		std::map<std::string, std::map<std::string, SimpleIni::IniLine> > m_IniMap;
		std::map<std::string, std::map<std::string, std::string> > m_DescriptionMap;
		std::string m_FileName;
		void SaveDescription(std::string section, std::string key, std::ofstream &file);
		void ParasitCar(std::string& str);
		std::string Trim(const std::string& str);
		std::string Normalize(const std::string& str);
		std::string m_OptionCommentCharacters;
};

/// \brief    Section iterator for SimpleIni class
/// \details  Class to browse sections of a configuration file load by SimpleIni class.
class SimpleIni::SectionIterator
{
	public:
		/// \brief    Constructor of a section iterator
		/// \details  Constructor to declare a section iterator
		SectionIterator();
		/// \brief    Constructor of a section iterator
		/// \details  Constructor to declare a section iterator, call by SimpleIni::BeginSection
		SectionIterator(std::map<std::string, std::map<std::string, SimpleIni::IniLine> >::iterator mapIterator);
		/// \brief    Overloading dereference operator
		/// \details  Overloading the dereference operator to get the section's name
		const std::string& operator*();
		/// \brief    Overloading pre-increment operator
		/// \details  Overloading the pre-increment operator to get the next section
		SectionIterator operator++();
		/// \brief    Overloading comparison operator ==
		/// \details  Overloading the comparison operator == to control the browse
		bool operator==(SectionIterator const& a);
		/// \brief    Overloading comparison operator !=
		/// \details  Overloading the comparison operator != to control the browse
		bool operator!=(SectionIterator const& a);

	private:
		std::map<std::string, std::map<std::string, SimpleIni::IniLine> >::iterator m_mapIterator;
};

/// \brief    Key iterator for SimpleIni class
/// \details  Class to browse keys of a configuration file load by SimpleIni class.
class SimpleIni::KeyIterator
{
	public:
		/// \brief    Constructor of a key iterator
		/// \details  Constructor to declare a key iterator
		KeyIterator();
		/// \brief    Constructor of a key iterator
		/// \details  Constructor to declare a key iterator, call by SimpleIni::BeginKey
		KeyIterator(std::map<std::string, SimpleIni::IniLine>::iterator mapIterator);
		/// \brief    Overloading dereference operator
		/// \details  Overloading the dereference operator to get the key's name
		const std::string& operator*();
		/// \brief    Overloading pre-increment operator
		/// \details  Overloading the pre-increment operator to get the next key
		KeyIterator operator++();
		/// \brief    Overloading comparison operator ==
		/// \details  Overloading the comparison operator == to control the browse
		bool operator==(KeyIterator const& a);
		/// \brief    Overloading comparison operator !=
		/// \details  Overloading the comparison operator != to control the browse
		bool operator!=(KeyIterator const& a);

	private:
		std::map<std::string, SimpleIni::IniLine>::iterator m_mapIterator;
};

#endif // SIMPLEINI_H
