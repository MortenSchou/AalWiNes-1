/* 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Peter G. Jensen
 */

/* 
 * File:   Moped.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 24, 2019, 9:19 PM
 */

#include "Moped.h"
#include "aalwines/utils/system.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

namespace pdaaal
{

    Moped::Moped() : _verification_time(false) {
        _tmpfilepath = boost::filesystem::unique_path().native();
        std::fstream file;
        file.open(_tmpfilepath, std::fstream::out);
        file.close();
    }

    Moped::~Moped() {
        boost::filesystem::remove(_tmpfilepath);
    }

    bool Moped::verify(const std::string& tmpfile, bool build_trace, bool use_pre)
    {
        // system call here!
        auto moped = get_env_var("MOPED_PATH");
        if(moped.empty())
        {
            throw base_error("MOPED_PATH environment variable not set");
        }
        std::stringstream cmd;
        cmd << moped;
        if(build_trace)
            cmd << " -t";
        if(use_pre)
            cmd << " -pB";
        cmd << " -s0 " << tmpfile;
        cmd << " -r DONE:_";
        auto cstr = cmd.str();
        _verification_time.start();
        auto res = exec(cstr.c_str());
        _verification_time.stop();
        return parse_result(res, build_trace);
    }
    
    bool Moped::parse_result(const std::string& result, bool build_trace)
    {
        bool res = false;
        std::istringstream is(result);
        std::string buffer;
        getline(is, buffer);
        if(buffer == "YES.")
        {
            res = true;
        }
        else if(buffer == "NO.")
        {
            res = false;
        }
        else
        {
            std::stringstream e;
            e << "An error occurred during verification using moped\n\n" << result;
            throw base_error(e.str());
        }
        _raw_trace.clear();
        if(build_trace && res)
        {
            bool saw_start = false;
            bool saw_init = false;
            bool saw_end = false;
            while(std::getline(is, buffer))
            {
                if(buffer.empty()) continue;
                if(saw_start)
                {
                    if(buffer[0] == 'D' || buffer[2] == '[') continue;
                    else if(buffer[0] == 'I' && !saw_init && !saw_end)
                    {
                        saw_init = true;
                    }
                    else if(buffer.substr(0, 11) == "--- END ---")
                    {
                        saw_end = true;
                        break;
                    }
                    else if(buffer[0] == 'S' && saw_init && !saw_end)
                    {
                        _raw_trace.push_back(buffer);
                    }
                    else
                    {
                        std::stringstream e;
                        e << "An error occurred during parsing of trace from moped\n\n";
                        if(saw_init) e << "\"I <_>\" seen\n";
                        if(saw_end) e << "\"DONE <_>\" seen\n";
                        e << buffer << "\n\n" << result;
                        throw base_error(e.str());
                    }
                }
                else if(buffer.substr(0, 13) == "--- START ---")
                {
                    saw_start = true;
                }
            }
            if(!saw_end)
            {
                std::stringstream e;
                e << "Missing accepting state in trace from Moped\n\n" << result;
                throw base_error(e.str());                                        
            }
        }
        std::getline(is, buffer);
        if(!buffer.empty()){
            _moped_verification_time_output = std::atof(buffer.c_str());
        } else {
            _moped_verification_time_output = _verification_time.duration();
            std::cerr << "The provided Moped engine does not support engine verification time." << std::endl;
        }
        return res;
    }

}

