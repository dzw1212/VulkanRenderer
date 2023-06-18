#pragma once
#include <memory>
#include "spdlog/spdlog.h"

class Log
{
private:
	Log() {}
	Log(const Log&) = delete;
	Log& operator=(const Log&) = delete;

	static std::shared_ptr<spdlog::logger> m_FileLogger;
	static std::shared_ptr<spdlog::logger> m_ConsoleLogger;
public:
	//static std::shared_ptr<spdlog::logger> GetLogger() { return m_Logger; };

	static void Critical(const std::string& strMsg) { m_FileLogger->critical(strMsg); m_ConsoleLogger->critical(strMsg); }
	static void Error(const std::string& strMsg) { m_FileLogger->error(strMsg); m_ConsoleLogger->error(strMsg); }
	static void Info(const std::string& strMsg) { m_FileLogger->info(strMsg); m_ConsoleLogger->info(strMsg); }

};