#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
using namespace std;

namespace KOM
{
	typedef struct tEntry 
	{
		std::string FileName;
		uint32_t	Size = 0;
		uint32_t	CompressedSize = 0;
		std::string Checksum;
		std::string FileTime;
		uint8_t		Algorithm = 0;

		uint32_t	_Offset;
		std::string _Buffer;
	} Entry;

	typedef struct tKOM 
	{
		struct tHeader // 72 bytes
		{
			std::string Header;
			uint32_t	EntryCount;
			uint32_t	IsCompressed;
			uint32_t	CRC;
			uint32_t	XMLSize;
			std::string XML;
		} Header;
		
		vector<Entry*> Entries;
	} Package;

	class Extractor
	{
	private:

	protected:

		std::iostream* m_stream = nullptr;
		Package* m_kom;

	public:

		Extractor(std::string _path);
		Extractor(std::fstream* _stream);
		~Extractor();

		bool Open(std::string _path);
		bool Open(std::fstream* _stream);

		Package Process();

		std::string PostProcess(tEntry* _entry);

	};

	class Kompactor 
	{
	private:

	protected:

		std::iostream* m_stream = nullptr;
		Package* m_kom;

	public:

		Kompactor(std::string _outPath);
		Kompactor(Package* _kom, std::string _outPath);
		~Kompactor();

		std::string MakeXML();

		void Process(vector<tEntry*> _entries);
		void Process(vector<string> _paths);
		void Process(string _root);

	};
}