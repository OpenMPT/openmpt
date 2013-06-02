/*
 * Logging.h
 * ---------
 * Purpose: General logging and user confirmation support
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


enum LogLevel
{
	/*LogDebug        = 1,*/
	LogNotification = 2,
	LogInformation  = 3,
	LogWarning      = 4,
	LogError        = 5
};


inline std::string LogLevelToString(LogLevel level)
{
	switch(level)
	{
	case LogError:        return "error";   break;
	case LogWarning:      return "warning"; break;
	case LogInformation:  return "info";    break;
	case LogNotification: return "notify";  break;
	}
	return "unknown";
}


class ILog
{
public:
	virtual void AddToLog(const std::string &text) const { AddToLog(LogInformation, text); }
	virtual	void AddToLog(LogLevel level, const std::string &text) const = 0;
};

