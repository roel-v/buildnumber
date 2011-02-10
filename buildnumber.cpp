#include "stdafx.h"

#include <wtypes.h>
#include <io.h>

#include <iostream>
#include <fstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/date_time/date.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "help.txt"

#define PUGAPI_VARIANT 0x58475550
#define PUGAPI_VERSION_MAJOR 1
#define PUGAPI_VERSION_MINOR 2

#include "pugxml\pugxml.h"
#include <boost\program_options.hpp>

const char buildnumber_version[] = "1.3";

namespace po = boost::program_options;

#define NODEVALUE(elementname) \
	parser.document().first_element_by_name(#elementname).first_node(pug::node_pcdata).value()

struct ProgLangBase
{
	virtual std::string Format() = 0;
	std::string m_VarName;
	std::string m_Version;
	std::string m_Id;

	static std::vector<ProgLangBase*> m_ProgLangList;
	static void AddProgLang(ProgLangBase* plb)
	{
		m_ProgLangList.push_back(plb);
	}
	static void Cleanup()
	{
		for (unsigned int i = 0 ; i < m_ProgLangList.size() ; i++) {
			delete m_ProgLangList[i];
		}
	}
	static ProgLangBase* FindProgLang(std::string id)
	{
		for (unsigned int i = 0 ; i < m_ProgLangList.size() ; i++) {
			if (m_ProgLangList[i]->m_Id == id) {
				return m_ProgLangList[i];
			}
		}

		return NULL;
	}
};

std::vector<ProgLangBase*> ProgLangBase::m_ProgLangList;

struct ProgLangC : public ProgLangBase
{
	ProgLangC() { m_Id = "c"; }
	virtual std::string Format()
	{
		return "const char " + m_VarName + "[] = \"" + m_Version + "\";";
	}
};

struct ProgLangDefault : public ProgLangBase
{
	ProgLangDefault() { m_Id = "none"; }
	virtual std::string Format()
	{
		return m_Version;
	}
};

std::string get_file_contents(std::string outputfile)
{
	std::string result;

	std::ifstream ifstream(outputfile.c_str());
	std::copy(std::istreambuf_iterator<char>(ifstream), std::istreambuf_iterator<char>(), std::back_inserter(result));

	return result;
}

std::string get_svn_revision()
{
	
	return "";
}

std::string expand_identifier_string(const std::string& original)
{
	std::string result;

	result = original;

	// Get current day and format it
	boost::gregorian::date today = boost::gregorian::day_clock::local_day();
	boost::gregorian::date_facet* date_facet = new boost::gregorian::date_facet("%Y-%m-%d");
	std::ostringstream date_ostr;	
	date_ostr.imbue(std::locale(std::cout.getloc(), date_facet));
	date_ostr << today;
	std::string current_date = date_ostr.str();

	// Get current time and format it
	boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_facet* time_facet = new boost::posix_time::time_facet("%H:%M:%S");
	std::ostringstream time_ostr;
	time_ostr.imbue(std::locale(std::cout.getloc(), time_facet));
	time_ostr << now;
	std::string current_time = time_ostr.str();

	// Since 'svn info' can take long, we only do that if needed.
	bool has_rev_placeholder = boost::find_first(original, "%r");
	if (has_rev_placeholder) {
		std::string svn_revision = get_svn_revision();
		boost::replace_all(result, "%r", svn_revision);
	}

	boost::replace_all(result, "%d", current_date);
	boost::replace_all(result, "%t", current_time);

	return result;
}

void do_display(std::string filename/*, std::ostream* output*/, char option, std::string programming_language, std::string variable_name, std::string outputfile, bool force, bool show_identifier)
{
	if (option != 'a'
			&& option != 'i'
			&& option != 'p'
			&& option != 'b'
			&& option != 'f'
			&& option != 's') {
		throw po::invalid_command_line_syntax("Wrong parameter to --display option", po::invalid_command_line_syntax::missing_parameter);
	}

	ProgLangBase* proglang = ProgLangBase::FindProgLang(programming_language);
	if (proglang == NULL) {
		throw po::validation_error(po::validation_error::invalid_option_value, programming_language, "--programming-language");
	}

	std::ostringstream versionstring;

	pug::xml_parser parser;
	parser.parse_file(filename.c_str());
	switch (option) {
		case 'a':
			versionstring << NODEVALUE(major_version);
			break;
		case 'i':
			versionstring << NODEVALUE(minor_version);
			break;
		case 'p':
			versionstring << NODEVALUE(patchlevel);
			break;
		case 'b':
			versionstring << NODEVALUE(build);
			break;
		case 'd':
			versionstring << NODEVALUE(identifier);
			break;
		case 'f':
			{
			std::string identifier = NODEVALUE(identifier);
			identifier = expand_identifier_string(identifier);
			versionstring << NODEVALUE(major_version)
				<< "." << NODEVALUE(minor_version)
				<< "." << NODEVALUE(patchlevel)
				<< "." << NODEVALUE(build);
			if (identifier != "" && show_identifier) {
				versionstring << "-" << identifier;
			}
			}
			break;
		case 's':
			{
			std::string identifier = NODEVALUE(identifier);
			identifier = expand_identifier_string(identifier);
			versionstring << NODEVALUE(major_version)
				<< "." << NODEVALUE(minor_version)
				<< "." << NODEVALUE(patchlevel);
			if (identifier != "" && show_identifier) {
				versionstring << "-" << identifier;
			}
			}
			break;
	}

	proglang->m_VarName = variable_name;
	proglang->m_Version = versionstring.str();

	// If --force was specified, we can just go ahead and write to the file.
	// Otherwise we have to check first if what we're about to write isn't the
	// same as what's already in the file.

	if (outputfile == "") {
		std::cout << proglang->Format();
	} else if (force) {
		std::ofstream output(outputfile.c_str(), std::ios::out);
		output << proglang->Format();
	} else {
		std::string oldtext = get_file_contents(outputfile);
		std::string newtext = proglang->Format();
		if (newtext != oldtext) {
			std::ofstream output(outputfile.c_str(), std::ios::out);
			output << newtext;
		}
	}
}

void do_increment(std::string filename, char option)
{
	if (option != 'a' && option != 'i' && option != 'p' && option != 'b') {
		throw po::invalid_command_line_syntax("Wrong parameter to --increment option", po::invalid_command_line_syntax::missing_parameter);
	}

	pug::xml_parser parser;
	parser.parse_file(filename.c_str());

#define ADD_ONE_TO(char, elementname) \
	case #@char:{ \
		std::string varvalue = NODEVALUE(elementname); \
		if (varvalue == "") { varvalue = "0"; }			\
		int new##elementname##1 = boost::lexical_cast<int>(varvalue) + 1; \
			parser.document().first_element_by_name(#elementname).first_node(pug::node_pcdata).value(boost::lexical_cast<std::string>(new##elementname##1).c_str()); \
			} break;

	switch (option) {
		ADD_ONE_TO(a, major_version)
		ADD_ONE_TO(i, minor_version)
		ADD_ONE_TO(p, patchlevel)
		ADD_ONE_TO(b, build)
	}
#undef CONVERT

	std::ofstream ofs(filename.c_str(), std::ios_base::out);
	parser.document().outer_xml(ofs);
}

void do_reset(std::string filename, char option)
{
	if (option != 'a' && option != 'i' && option != 'p' && option != 'b') {
		throw po::invalid_command_line_syntax("Wrong parameter to --reset option", po::invalid_command_line_syntax::missing_parameter);
	}

	pug::xml_parser parser;
	parser.parse_file(filename.c_str());

#define RESET(char, elementname) \
	case #@char:{ \
		parser.document().first_element_by_name(#elementname).first_node(pug::node_pcdata).value("0"); \
		} break;

	switch (option) {
		RESET(a, major_version)
		RESET(i, minor_version)
		RESET(p, patchlevel)
		RESET(b, build)
	}
#undef RESET

	std::ofstream ofs(filename.c_str(), std::ios_base::out);
	parser.document().outer_xml(ofs);
}

void do_create(std::string filename)
{
	pug::xml_parser parser;
	parser.create();
	pug::xml_node newnode = parser.document().append_child(pug::node_element);
	newnode.name("version");

	pug::xml_node majornode = newnode.append_child(pug::node_element);
	majornode.append_child(pug::node_pcdata).value("0");
	majornode.name("major_version");
	pug::xml_node minornode = newnode.append_child(pug::node_element);
	minornode.append_child(pug::node_pcdata).value("0");
	minornode.name("minor_version");
	pug::xml_node patchlevelnode = newnode.append_child(pug::node_element);
	patchlevelnode.append_child(pug::node_pcdata).value("0");
	patchlevelnode.name("patchlevel");
	pug::xml_node buildnode = newnode.append_child(pug::node_element);
	buildnode.append_child(pug::node_pcdata).value("0");
	buildnode.name("build");
	pug::xml_node identifiernode = newnode.append_child(pug::node_element);
	identifiernode.append_child(pug::node_pcdata);//.value("");
	identifiernode.name("identifier");

	std::ofstream ofs(filename.c_str(), std::ios_base::out);
	parser.document().outer_xml(ofs);
}

void show_short_usage()
{
	std::cout << SHORT_USAGE << std::endl;
}

int main(int argc, char* argv[])
{
	std::string f1;
	po::options_description desc(INTRODUCTION_TEXT);
	desc.add_options()
		("help,h", HELP_TEXT)
		("version,v", VERSION_TEXT)
		("file,f", po::value<std::string>(&f1)->default_value("version.xml"), FILE_TEXT)
		("display,d", po::value<char>(), DISPLAY_TEXT)
		("no-identifier,n", NO_IDENTIFIER_TEXT)
		("increment,i", po::value<char>(), INCREMENT_TEXT)
		("reset,r", po::value<char>(), RESET_TEXT)
		("create,c", CREATE_TEXT)
		("output,o", po::value<std::string>(), OUTPUT_TEXT)
		("programming-language,p", po::value<std::string>()->default_value("none"), PROGRAMMING_LANGUAGE_TEXT)
		("variable-name,a", po::value<std::string>()->default_value("version"), VARIABLE_NAME_TEXT)
		("force", FORCE_TEXT)
	;

	po::variables_map varmap;
	try {
		po::store(po::parse_command_line(argc, argv, desc), varmap);
	}
	catch (po::invalid_command_line_syntax e) {
		std::cout << "Invalid command line syntax." << std::endl;	
		std::cout << "Error: " << e.what() << std::endl;
		show_short_usage();
		return 1;
	}
	catch (po::unknown_option e) {
		std::cout << e.what() << std::endl;
		show_short_usage();
		return 1;
	}
	po::notify(varmap);

	//std::ostream* output = NULL;

	bool force = false;
	if (varmap.count("force")) {
		// Force is only valid when --output is also specified.
		if (!varmap.count("output")) {
			std::cout << "--force is only valid when --output is also specified." << std::endl;
			return 1;
		}
		force = true;
	}

	//std::string oldtext;

	// If an output file is specified, use that to write to; otherwise use stdout.
	std::string outputfile = "";
	if (varmap.count("output")) {
		outputfile = varmap["output"].as<std::string>();
	}

	// User wants to see help text?
	if (varmap.count("help")) {
		std::cout << desc << std::endl;
		return 1;
	}
	// User wants to know version?
	if (varmap.count("version")) {
		std::cout << buildnumber_version << std::endl;
		return 1;
	}

	bool show_identifier = true;
	if (varmap.count("no-identifier") != 0) {
		show_identifier = false;
	}

	try {
		std::string file = varmap["file"].as<std::string>();
		if (varmap.count("create") == 0 && ::_access(file.c_str(), 0)) {
			std::cout << "File " << varmap["file"].as<std::string>() << " not found." << std::endl;
			return 1;
		}

		if (varmap.count("display")) {
			ProgLangBase::AddProgLang(new ProgLangC);
			ProgLangBase::AddProgLang(new ProgLangDefault);
			do_display(file,
				//output,
				varmap["display"].as<char>(),
				varmap["programming-language"].as<std::string>(),
				varmap["variable-name"].as<std::string>(),
				outputfile,
				force,
				show_identifier);
			ProgLangBase::Cleanup();
		} else if (varmap.count("increment")) {
			do_increment(file, varmap["increment"].as<char>());
		} else if (varmap.count("reset")) {
			do_reset(file, varmap["reset"].as<char>());
		} else if (varmap.count("create")) {
			do_create(file);
		} else {
			ProgLangBase::AddProgLang(new ProgLangC);
			ProgLangBase::AddProgLang(new ProgLangDefault);
			do_display(file, 'f',
				varmap["programming-language"].as<std::string>(),
				varmap["variable_name"].as<std::string>(), outputfile, false, show_identifier);
			ProgLangBase::Cleanup();
		}
	}
	catch (boost::bad_any_cast) {
		std::cout << "Error in parameter format." << std::endl;	
		show_short_usage();
		return 1;
	}
	catch (po::invalid_command_line_syntax e) {
		std::cout << "Invalid command line syntax." << std::endl;	
		std::cout << "Error: " << e.what() << std::endl;
		show_short_usage();
		return 1;
	}
	catch (po::validation_error e) {
		std::cout << "Error in parameters." << std::endl;
		// @todo: why is what() private?
		//std::cout << "Error: " << e.what() << std::endl;
		show_short_usage();
		return 1;
	}

	/*if (output != &std::cout) {
		delete output;
	}*/

	return 0;
}
