/*
 * mptWine.h
 * ---------
 * Purpose: Wine stuff.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mptOS.h"
#include "mpt/library/library.hpp"
#include "openmpt/base/FlagSet.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


#if MPT_OS_WINDOWS


namespace mpt
{

namespace Wine
{


class Exception
 : public std::runtime_error
{
public:
	Exception(const std::string &text)
	 : std::runtime_error(text)
	{
		return;
	}
};


typedef void (*ExecutePosixCommandProgress)(void *userdata);

enum ExecuteProgressResult
{
	ExecuteProgressContinueWaiting = 0,
	ExecuteProgressAsyncCancel = -1,
};

typedef ExecuteProgressResult (*ExecutePosixShellScriptProgress)(void *userdata);


enum ExecFlags
{
	ExecFlagNone           = 0,
	ExecFlagSilent         = 1<<0, // do not show a terminal window
	ExecFlagInteractive    = 1<<1, // allow interaction (prevents stdout and stderr capturing and implies !silent)
	ExecFlagAsync          = 1<<2, // do not wait for the script to finish
	ExecFlagProgressWindow = 1<<3, // not implemented by mptOS
	ExecFlagSplitOutput    = 1<<4, // split stdout and stderr (implies silent)
	ExecFlagsDefault       = ExecFlagNone
};
MPT_DECLARE_ENUM(ExecFlags)

struct ExecResult
{
	int exitcode;
	std::string output;
	std::string error;
	std::map<std::string, std::vector<char> > filetree;
	static ExecResult Error()
	{
		ExecResult result;
		result.exitcode = -1;
		return result;
	}
};


class Context
{
protected:
	mpt::OS::Wine::VersionContext m_VersionContext;
	std::shared_ptr<std::optional<mpt::library>> m_Kernel32;
private:
	LPWSTR (*CDECL wine_get_dos_file_name)(LPCSTR str);
	LPSTR (*CDECL wine_get_unix_file_name)(LPCWSTR str);
protected:
	std::string m_Uname_m;
	std::string m_HOME;
	std::string m_XDG_DATA_HOME;
	std::string m_XDG_CACHE_HOME;
	std::string m_XDG_CONFIG_HOME;
public:
	Context(mpt::OS::Wine::VersionContext versionContext);
public:
	std::string EscapePosixShell(std::string line);
	std::string PathToPosix(mpt::PathString windowsPath);
	mpt::PathString PathToWindows(std::string hostPath);
	ExecResult ExecutePosixShellScript(std::string script, FlagSet<ExecFlags> flags, std::map<std::string, std::vector<char> > filetree, std::string title, ExecutePosixCommandProgress progress, ExecutePosixShellScriptProgress progressCancel, void *userdata);
	int ExecutePosixShellCommand(std::string command, std::string & output, std::string & error);
	std::string PathToPosixCanonical(mpt::PathString windowsPath);
	std::string GetPosixEnvVar(std::string var, std::string def = std::string());
public:
	mpt::OS::Wine::VersionContext VersionContext() const { return m_VersionContext; }
	std::shared_ptr<std::optional<mpt::library>> Kernel32() const { return m_Kernel32; }
	std::string Uname_m() const { return m_Uname_m; }
	std::string HOME() const { return m_HOME; }
	std::string XDG_DATA_HOME() const { return m_XDG_DATA_HOME; }
	std::string XDG_CACHE_HOME() const { return m_XDG_CACHE_HOME; }
	std::string XDG_CONFIG_HOME() const { return m_XDG_CONFIG_HOME; }
};


} // namespace Wine

} // namespace mpt


#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_END
