/*
 * MPTrackUtilWine.h
 * -----------------
 * Purpose: Wine utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "../common/mptWine.h"


OPENMPT_NAMESPACE_BEGIN


namespace Util
{


namespace Wine
{


mpt::Wine::ExecResult ExecutePosixShellScript(mpt::Wine::Context & wine, std::string script, FlagSet<mpt::Wine::ExecFlags> flags, std::map<std::string, std::vector<char> > filetree, std::string title, std::string status);


class Dialog
{
private:
	bool m_Terminal;
	std::string	m_Title;
private:
	std::string DialogVar() const;
public:
	Dialog(std::string title, bool terminal);
	std::string Title() const;
	std::string Detect() const;
	std::string Status(std::string text) const;
	std::string MessageBox(std::string text) const;
	std::string YesNo(std::string text) const;
	std::string TextBox(std::string filename) const;
	std::string Progress(std::string text) const;
};


} // namespace Wine


} // namespace Util


OPENMPT_NAMESPACE_END
