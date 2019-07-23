// Created: 2016-11-10
//
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is a part of CADRays software.
//
// CADRays is free software; you can use it under the terms of the MIT license,
// refer to file LICENSE.txt for complete text of the license and disclaimer of 
// any warranty.

#ifndef __Settings_H__
#define __Settings_H__

#include <map>
#include <string>

class Settings
{
public:
    //! Construct Settings and parse given filename. See ini.h for more info
    //! about the parsing.
    Settings(const std::string& filename);

    //! Return the result of ini_parse(), i.e., 0 on success, line number of
    //! first error on parse error, or -1 on file open error.
    int ParseError() const;

    //! Get a string value from INI file, returning default_value if not found.
    std::string Get(const std::string& section, const std::string& name,
                    const std::string& default_value) const;

    //! Set a string value to INI file.
    void Set(const std::string& section, const std::string& name, const std::string& value);

    //! Set an integer value to INI file.
    void SetInteger(const std::string& section, const std::string& name, int value);

    //! Set a floating point value to INI file.
    void SetReal(const std::string& section, const std::string& name, double value);

    //! Set a Boolean value to INI file.
    void SetBoolean(const std::string& section, const std::string& name, bool value);

    //! Get an integer (long) value from INI file, returning default_value if
    //! not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long GetInteger(const std::string& section, const std::string& name, long default_value) const;

    //! Get a real (floating point double) value from INI file, returning
    //! default_value if not found or not a valid floating point value
    //! according to strtod().
    double GetReal(const std::string& section, const std::string& name, double default_value) const;

    //! Get a boolean value from INI file, returning default_value if not found or if
    //! not a valid true/false value. Valid true values are "true", "yes", "on", "1",
    //! and valid false values are "false", "no", "off", "0" (not case sensitive).
    bool GetBoolean(const std::string& section, const std::string& name, bool default_value) const;

    //! Dumps contents of map into INI file.
    void Dump (const std::string& filename);

private:
    int _error;
    std::map<std::pair<std::string, std::string>, std::string> _values;
    static std::pair<std::string, std::string> MakeKey(const std::string& section, const std::string& name);
    static int ValueHandler(void* user, const char* section, const char* name,
                            const char* value);
};

#endif  // __Settings_H__
