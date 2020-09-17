#include "KOM/KOM.h"
#include "Utils/unluac.h"
#include "Console/Args.h"

inline void Save(std::string _path, std::string _buffer) 
{
	fstream _handle(_path, std::ios::binary | std::ios::out);
	_handle.write(&_buffer[0], _buffer.length());
	_handle.close();
}

int main(int argc, char* argv[]) 
{
	cxxopts::Options options(argv[0], " - ElSword KOM Archiver");
	options.add_options()
		("u,unpack", "Unpack targeted KOM.", cxxopts::value<std::string>())
		("o,out", "Targeted output path.", cxxopts::value<std::string>())
		("p,pack", "Pack targeted directory/file.", cxxopts::value<std::string>())
		("r,recompile", "Use luac.exe to recompile files before rearchiving.", cxxopts::value<bool>()->default_value("false"))
		("d,decompile", "Use unluac.jar to decompile files after extraction.", cxxopts::value<bool>()->default_value("false"));
	auto result = options.parse(argc, argv);

	if (result["out"].count() != 1) 
	{
		std::cout << "You need to specify the output path with -o first!";
		return 1;
	}

	if (!(result["pack"].count() == 1))
		CreateDirectoryA(result["out"].as<string>().c_str(), NULL);

	std::string _outPath = result["out"].as<string>();
	if (_outPath.substr(_outPath.length() - 1, _outPath.length()).compare("/") != 0 ||
		_outPath.substr(_outPath.length() - 1, _outPath.length()).compare("\\") != 0) 
	{
		_outPath.append("/");
	}

	if (result["unpack"].count() == 1) 
	{
		KOM::Extractor* _extractor = new KOM::Extractor(result["unpack"].as<string>());
		auto _kom = _extractor->Process();

		std::string _path = _outPath;
		for (auto _file = _kom.Entries.begin(); _file != _kom.Entries.end(); _file++)
		{
			std::cout << (*_file)->FileName << "\n";
			auto _postProcessed = _extractor->PostProcess(*_file);

			Save(_path + (*_file)->FileName, _postProcessed);
		}

		if (result["decompile"].count() == 1 && result["decompile"].as<bool>())
			if (Utils::IsJarInstalled())
				Utils::Decompile(_outPath);

	} else if (result["pack"].count() == 1) {

		if (result["recompile"].count() == 1 && result["recompile"].as<bool>())
			if (Utils::IsLuacInstalled())
				Utils::Compile(result["pack"].as<string>());

		KOM::Kompactor* _kompactor = new KOM::Kompactor(result["out"].as<string>());
		_kompactor->Process(result["pack"].as<string>());
		_kompactor->~Kompactor();
	}

	getchar();
}