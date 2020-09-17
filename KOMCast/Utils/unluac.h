#pragma once

#include <fstream>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <Windows.h>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
using namespace std;

namespace Utils 
{
	static std::string Exec(std::string command)
    {
        char buffer[65565];
        string result = "";

        // Open pipe to file
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            return "popen failed!";
        }

        // read till end of process:
        while (!feof(pipe)) {

            // use buffer to read and add to result
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }

        _pclose(pipe);
        return result;
    }

    static bool IsJarInstalled() 
    {
        return std::experimental::filesystem::exists("./unluac.jar");
    }

    static bool IsLuacInstalled() 
    {
        return std::experimental::filesystem::exists("./luac.exe");
    }

	static void Decompile(std::string _dir)
	{
        for (auto& file : experimental::filesystem::directory_iterator(_dir)) 
        {
            auto _file = file.path().generic_string();
            auto _type = _file.substr(_file.length() - 3, _file.length());

            if (_type.compare("lua") == 0)
            {
                std::string _output = Utils::Exec("java -Xmx1024m -jar unluac.jar ./" + _file);
                if (!_output.empty())
                {
                    DeleteFileA(_file.c_str());
                    fstream _handle(_file.c_str(), std::ios::binary | std::ios::out);
                    _handle.write((char*)&_output[0], _output.length());
                    _handle.close();
                }
            }
        }
	}

    static void Compile(std::string _dir)
    {
        for (auto& file : experimental::filesystem::directory_iterator(_dir))
        {
            auto _file = file.path().generic_string();
            auto _type = _file.substr(_file.length() - 3, _file.length());

            if (_type.compare("lua") == 0)
            {
                std::string _output = Utils::Exec("luac.exe ./" + _file);
                if (std::experimental::filesystem::exists("./luac.out")) 
                {
                    DeleteFileA(_file.c_str());
                    MoveFileA("./luac.out", _file.c_str());
                }
            }
        }
    }

}