// This file is part of CaesarIA.
//
// CaesarIA is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CaesarIA is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CaesarIA.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#include "logger.hpp"
#include "requirements.hpp"
#include "utils.hpp"
#include "time.hpp"
#include "foreach.hpp"
#include "list.hpp"

#include <cstdarg>
#include <cfloat>
#include <stdio.h>
#include <climits>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <fstream>
#include <map>
#include "vfs/directory.hpp"

#ifdef CAESARIA_PLATFORM_ANDROID
#include <android/log.h>
#include <SDL_system.h>
#endif

class FileLogWriter : public LogWriter
{
private:
  FILE* _logFile;
public:
  FileLogWriter(const std::string& path)
  {
    DateTime t = DateTime::currenTime();

    _logFile = fopen(path.c_str(), "w");

    if( _logFile )
    {
      fputs("Caesaria logfile created: ", _logFile);
      fputs( utils::format( 0xff, "%02d:%02d:%02d",
             t.hour(), t.minutes(), t.seconds()).c_str(),
             _logFile);
      fputs("\n", _logFile);
    }
  }

  ~FileLogWriter()
  {
    DateTime t = DateTime::currenTime();

    if( _logFile )
    {
      fputs("Caesaria logfile closed: ", _logFile);
      fputs( utils::format( 0xff, "%02d:%02d:%02d",
             t.hour(), t.minutes(), t.seconds()).c_str(),
             _logFile);
      fputs("\n", _logFile);

      fflush(_logFile);
    }
  }

  virtual bool isActive() const { return _logFile != 0; }

  virtual void write( const std::string& str, bool )
  {
    // Don't write progress stuff into the logfile
    // Make sure only one thread is writing to the file at a time
    static int count = 0;
    if( _logFile )
    {
      fputs(str.c_str(), _logFile);
      fputs("\n", _logFile);

      count++;
      if( count % 10 == 0 )
      {
        fflush(_logFile);
      }
    }
  }
};

class ConsoleLogWriter : public LogWriter
{
public:
  virtual void write( const std::string& str, bool newline )
  {
#ifdef CAESARIA_PLATFORM_ANDROID    
    __android_log_print(ANDROID_LOG_DEBUG, CAESARIA_PLATFORM_NAME, "%s", str.c_str() );
    if( newline )
      __android_log_print(ANDROID_LOG_DEBUG, CAESARIA_PLATFORM_NAME, "\n" );
#else
    std::cout << str;
    if( newline ) std::cout << std::endl;
    else std::cout << std::flush;
#endif
  }

  virtual bool isActive() const { return true; }
};

class Logger::Impl
{
public:
  typedef std::map<std::string,LogWriterPtr> Writers;
  typedef List<std::string> Filters;

  Filters filters;

  Writers writers;

  void write( const std::string& message, bool newline=true )
  {
    // Check for filter pass
    bool pass = filters.size() == 0;
    for( auto& filter : filters )
    {
      if (message.compare( 0, filter.length(), filter ) == 0)
      {
        pass = true;
        break;
      }
    }
    if (!pass) return;

    for( auto&& item : writers )
    {
      if( item.second.isValid() )
      {
        item.second->write( message, newline );
      }
    }
  }

};

void Logger::warning( const char* fmt, ... )
{
  va_list argument_list;

  va_start(argument_list, fmt);

  std::string ret;
  utils::vformat( ret, 512, fmt, argument_list );

  va_end(argument_list);

  instance()._d->write( ret );
}

void Logger::warning(const std::string& text) {  instance()._d->write( text );}
void Logger::warningIf(bool warn, const std::string& text){  if( warn ) warning( text ); }
void Logger::update(const std::string& text, bool newline){  instance()._d->write( text, newline ); }

void Logger::addFilter(const std::string& text)
{
  if (hasFilter(text)) return;
  instance()._d->filters.append(text);
}

bool Logger::hasFilter(const std::string& text)
{
  for( auto& filter : instance()._d->filters)
  {
    if (filter == text) return true;
  }
  return false;
}

bool Logger::removeFilter(const std::string& text)
{
  foreach(filter, instance()._d->filters)
  {
    if (*filter == text)
    {
      instance()._d->filters.erase(filter);
      return true;
    }
  }
  return false;
}

void Logger::registerWriter(Logger::Type type, const std::string& param )
{
  switch( type )
  {
  case consolelog:
  {
    LogWriterPtr wr( new ConsoleLogWriter() );
    wr->drop();
    registerWriter( "__console", wr );
  }
  break;

  case filelog:
  {
    vfs::Directory workdir( param );
    vfs::Path fullname = workdir/"stdout.txt";
    LogWriterPtr wr( new FileLogWriter( fullname.toString() ) );
    wr->drop();
    registerWriter( "__log", wr );
  }
  break;

  case count: break;
  }
}

Logger& Logger::instance()
{
  static Logger inst;
  return inst;
}

Logger::~Logger() {}

Logger::Logger() : _d( new Impl )
{
}

void Logger::registerWriter(std::string name, LogWriterPtr writer)
{
  if( writer.isValid() && writer->isActive() )
  {
    instance()._d->writers[ name ] = writer;
  }
}

void SimpleLogger::write(const std::string &message, bool newline) {
  Logger::update( message, newline );
}

SimpleLogger::SimpleLogger( const std::string& category)
  : _category(category)
{}

void SimpleLogger::llog(SimpleLogger::Severity severity, const std::string &text)
{
  std::string rtext = toS(severity) + " ";
  rtext += _category;
  rtext += ": " + text;
  write(rtext);
}

void SimpleLogger::vlog(SimpleLogger::Severity severity, const char *fmt, va_list args) {
  std::string ret;
  utils::vformat(ret, 512, fmt, args);
  llog(severity, ret);
}

void SimpleLogger::log(SimpleLogger::Severity severity, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::string ret;
  utils::vformat(ret, 512, fmt, args);
  llog(severity, ret);
  va_end(args);
}

bool SimpleLogger::isDebugEnabled() const {
#ifdef DEBUG
  return true;
#else
  return false;
#endif
}

const std::string SimpleLogger::toS(SimpleLogger::Severity severity) {
  switch (severity) {
    case Severity::DBG:
      return "[DEBUG]";
    case Severity::INFO:
      return "[INFO]";
    case Severity::WARN:
      return "[WARN]";
    case Severity::ERR:
      return "[ERROR]";
    case Severity::FATAL:
      return "[FATAL]";
  }
  return "[UNKNOWN]";
}

void SimpleLogger::debug(const std::string &text) {
  llog(Severity::DBG, text);
}

void SimpleLogger::info(const std::string &text) {
  llog(Severity::INFO, text);
}

void SimpleLogger::warn(const std::string &text) {
  llog(Severity::WARN, text);
}

void SimpleLogger::error(const std::string &text) {
  llog(Severity::ERR, text);
}

void SimpleLogger::fatal(const std::string &text) {
  llog(Severity::FATAL, text);
}

void SimpleLogger::debug(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vlog(Severity::DBG, fmt, args);
  va_end(args);
}

void SimpleLogger::info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vlog(Severity::INFO, fmt, args);
  va_end(args);
}

void SimpleLogger::warn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vlog(Severity::WARN, fmt, args);
  va_end(args);
}

void SimpleLogger::error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vlog(Severity::ERR, fmt, args);
  va_end(args);
}

void SimpleLogger::fatal(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vlog(Severity::FATAL, fmt, args);
  va_end(args);
}
