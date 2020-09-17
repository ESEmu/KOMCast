#include "KOM.h"

#include <rapidxml-ns/rapidxml_ns.hpp>
#include <rapidxml-ns/rapidxml_ns_print.hpp>

#define DEFAULT_HEADER "KOG GC TEAM MASSFILE V.0.3."

KOM::Extractor::Extractor(std::string _path)
{
	this->Open(_path);
}

KOM::Extractor::Extractor(std::fstream* _stream)
{
	this->Open(_stream);
}

KOM::Extractor::~Extractor()
{
	if (this->m_stream != nullptr) 
	{
		this->m_stream->flush();
		delete this->m_stream;
	}
}

bool KOM::Extractor::Open(std::string _path)
{
	return this->Open(new fstream(_path, std::ios::binary | std::ios::in));
}

bool KOM::Extractor::Open(std::fstream* _stream)
{
	_ASSERT(_stream != nullptr);
	if (_stream != nullptr && _stream->is_open()) 
	{
		this->m_stream = dynamic_cast<std::iostream*>(_stream);
		return true;
	} else
		throw std::exception("The file is not open");

	return false;
	_ASSERT(this->m_stream != nullptr);
}

KOM::Package KOM::Extractor::Process()
{
	if (this->m_stream != nullptr && this->m_stream->good()) 
	{
		this->m_kom = new KOM::Package();

		this->m_kom->Header.Header.resize(27);
		this->m_stream->read(&this->m_kom->Header.Header[0], 27);

		// TODO: Multi mass file version support? Implement an extractor implementation on enum template from this field.
		if (this->m_kom->Header.Header.compare(DEFAULT_HEADER) > 0)
			throw std::exception("Not a supported KOM file");

		this->m_stream->ignore(25);

		this->m_stream->read(reinterpret_cast<char*>(&this->m_kom->Header.EntryCount), sizeof(uint32_t));

		// Ignore, we will see that from the XML :)

		//if (!(this->m_kom->Header.EntryCount > 0))
		//	throw std::exception("Size of XML header is null");

		this->m_stream->read(reinterpret_cast<char*>(&this->m_kom->Header.IsCompressed), sizeof(uint32_t));
		this->m_stream->ignore(4); // Later this will become the BF's Key SEED
		this->m_stream->read(reinterpret_cast<char*>(&this->m_kom->Header.CRC), sizeof(uint32_t));
		this->m_stream->read(reinterpret_cast<char*>(&this->m_kom->Header.XMLSize), sizeof(uint32_t));

		if (!(this->m_kom->Header.XMLSize > 0))
			throw std::exception("Size of XML header is null");

		auto _malloced = (char*)malloc(this->m_kom->Header.XMLSize + 1);
		if (!_malloced) 
		{
			throw std::exception("Not enough memory...really?");
			return tKOM();
		}
		memset(_malloced, 0, this->m_kom->Header.XMLSize + 1);

		this->m_stream->read(reinterpret_cast<char*>(_malloced), this->m_kom->Header.XMLSize);

		rapidxml_ns::xml_document<> _document;
		_document.parse<rapidxml_ns::parse_non_destructive>(const_cast<char*>(_malloced));

		this->m_kom->Header.XML.assign(_malloced, this->m_kom->Header.XMLSize);

		auto _offset = 72 + this->m_kom->Header.XMLSize;
		// Open the Files node.
		rapidxml_ns::xml_node<>* _xmlRoot = _document.first_node("Files");
		// Go trough each File
		for (rapidxml_ns::xml_node<>* _xmlNode = _xmlRoot->first_node("File"); _xmlNode; _xmlNode = _xmlNode->next_sibling())
		{
			// Create an entry on our end.
			KOM::Entry* _entrySlave = new KOM::Entry();

			// Fetch data
			_entrySlave->FileName.assign(_xmlNode->first_attribute("Name")->value(), _xmlNode->first_attribute("Name")->value_size());
			_entrySlave->Checksum.assign(_xmlNode->first_attribute("CheckSum")->value(), _xmlNode->first_attribute("CheckSum")->value_size());
			_entrySlave->FileTime.assign(_xmlNode->first_attribute("FileTime")->value(), _xmlNode->first_attribute("FileTime")->value_size());

			_entrySlave->Size			= atoi(_xmlNode->first_attribute("Size")->value());
			_entrySlave->CompressedSize = atoi(_xmlNode->first_attribute("CompressedSize")->value());
			_entrySlave->Algorithm		= atoi(_xmlNode->first_attribute("Algorithm")->value());

			_entrySlave->_Offset = _offset;

			// Push to the list.
			this->m_kom->Entries.push_back(_entrySlave);
			
			// Increase the global offset.
			_offset += _entrySlave->CompressedSize;
			_entrySlave = nullptr;
		}
		
		if (_malloced)
			free(_malloced);

		return *this->m_kom;
	}
}

inline std::string __xor(std::string _input) 
{
	char _key[12] = { 0x02, 0xAA, 0xF8, 0xC6, 0xDC, 0xAB, 0x47, 0x26, 0xEF, 0xBB, 0x00, 0x98 };
	std::string _output;
	_output.resize(_input.size());

	for (auto i = 0; i < _input.length(); i++) 
		_output[i] = _input[i] ^ _key[i % 12];

	return _output;
}

#include "../Compression/Compression.h"
#include <time.h>
std::string KOM::Extractor::PostProcess(tEntry* _entry)
{
	if (!_entry)
		throw std::exception("The entry specified is null");

	char* _output = nullptr;
	std::string _input;
	std::string _output_final;

	this->m_stream->seekg(_entry->_Offset, std::ios_base::beg);
	_input.resize(_entry->CompressedSize);
	this->m_stream->read(reinterpret_cast<char*>(&_input[0]), _entry->CompressedSize);

	switch (_entry->Algorithm) 
	{
	case 0:
	default:

		auto _ret = zlib_decompress_mem(&_input[0], _entry->CompressedSize, &_output, _entry->Size);
		_input.clear();
#if !defined(Z_OK)
	#define Z_OK 0
#endif
		if (_ret == Z_OK || _ret == _entry->Size)
		{
			std::string buffer;
			buffer.assign(_output, _entry->Size);
			
			auto _ret = __xor(buffer);
			buffer.clear();
			free(_output);

			return _ret;
		}

		break;
	}

	return "";
}

KOM::Kompactor::Kompactor(std::string _outPath)
{
	this->m_stream = dynamic_cast<std::iostream*>(new fstream(_outPath.c_str(), std::ios::binary | std::ios::out));
	this->m_kom = new KOM::tKOM();

	if (this->m_stream == nullptr)
		throw std::exception("Could not open the file in write mode");
}

KOM::Kompactor::Kompactor(Package* _kom, std::string _outPath)
{
	this->m_stream = dynamic_cast<std::iostream*>(new fstream(_outPath.c_str(), std::ios::binary | std::ios::out));
	this->m_kom = _kom;

	if (this->m_stream == nullptr || _kom == nullptr)
		throw std::exception("Could not open the file in write mode");
}

KOM::Kompactor::~Kompactor()
{
	if (this->m_kom) {
		if (this->m_kom->Entries.size() > 0) 
			for (auto i = 0; i < this->m_kom->Entries.size(); i++)
				this->m_kom->Entries[i]->_Buffer.clear();

		free(this->m_kom);
	}

	if (this->m_stream)
		this->m_stream->flush();
}

std::string KOM::Kompactor::MakeXML()
{
	std::string _output;

	rapidxml_ns::xml_document<> doc;
	rapidxml_ns::xml_node<>* decl = doc.allocate_node(rapidxml_ns::node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	doc.append_node(decl);

	rapidxml_ns::xml_node<>* root = doc.allocate_node(rapidxml_ns::node_element, "Files");
	rapidxml_ns::xml_node<>* _file = nullptr;
	char* _slaveInt = nullptr;

	for (auto i = this->m_kom->Entries.begin(); i != this->m_kom->Entries.end(); i++) 
	{
		_file = doc.allocate_node(rapidxml_ns::node_element, "File");
		_file->append_attribute(doc.allocate_attribute("Name", (*i)->FileName.c_str()));
		_slaveInt = (char*)malloc(12);
		_file->append_attribute(doc.allocate_attribute("Size", _itoa((*i)->Size, _slaveInt, 10)));
		_slaveInt = (char*)malloc(12);
		_file->append_attribute(doc.allocate_attribute("CompressedSize", _itoa((*i)->CompressedSize, _slaveInt, 10)));
		_slaveInt = (char*)malloc(12);

		// Base 16 (Hex) checksum
		(*i)->Checksum = _itoa(adler32(adler32(0, Z_NULL, 0), (Bytef*)&(*i)->_Buffer[0], (*i)->CompressedSize), _slaveInt, 16);
		_file->append_attribute(doc.allocate_attribute("CheckSum", (*i)->Checksum.c_str()));
		_slaveInt = (char*)malloc(12);

		// Base 16 (Hex) filetime
		(*i)->FileTime = _itoa(time(0), _slaveInt, 16);
		_file->append_attribute(doc.allocate_attribute("FileTime", (*i)->FileTime.c_str()));
		_slaveInt = (char*)malloc(12);
		_file->append_attribute(doc.allocate_attribute("Algorithm", _itoa((*i)->Algorithm, _slaveInt, 10)));
		root->append_node(_file);
	}
	doc.append_node(root);

	rapidxml_ns::print(std::back_inserter(_output), doc);

	return _output;
}

void KOM::Kompactor::Process(vector<tEntry*> _entries)
{
	this->m_kom->Entries = _entries;
	if (!(_entries.size() > 0))
		throw std::exception("Can not create an empty KOM");

	this->m_kom->Header.Header = DEFAULT_HEADER;
	this->m_kom->Header.IsCompressed = 1;
	this->m_kom->Header.EntryCount = this->m_kom->Entries.size();
	char* _nullInt = (char*)malloc(25);
	memset(_nullInt, 0, 25);


	this->m_stream->write(&this->m_kom->Header.Header[0], this->m_kom->Header.Header.length());
	this->m_stream->write(reinterpret_cast<char*>(_nullInt), 25);
	
	this->m_stream->write(reinterpret_cast<char*>(&this->m_kom->Header.EntryCount), sizeof(uint32_t)); 
	this->m_stream->write(reinterpret_cast<char*>(&this->m_kom->Header.IsCompressed), sizeof(uint32_t));
	this->m_stream->write(reinterpret_cast<char*>(_nullInt), sizeof(uint32_t));

	this->m_kom->Header.XML = this->MakeXML();
	this->m_kom->Header.XMLSize = this->m_kom->Header.XML.length();

	this->m_kom->Header.CRC = adler32(adler32(0, Z_NULL, 0), (Bytef*)&this->m_kom->Header.XML[0], this->m_kom->Header.XML.length());
	this->m_stream->write(reinterpret_cast<char*>(&this->m_kom->Header.CRC), sizeof(uint32_t));

	this->m_stream->write(reinterpret_cast<char*>(&this->m_kom->Header.XMLSize), sizeof(uint32_t));
	this->m_stream->write(const_cast<char*>(this->m_kom->Header.XML.c_str()), this->m_kom->Header.XMLSize);

	auto _offset = 72 + this->m_kom->Header.XMLSize;

	for (auto i = _entries.begin(); i != _entries.end(); i++) 
	{
		this->m_stream->write(const_cast<char*>((*i)->_Buffer.c_str()), (*i)->CompressedSize);
		(*i)->_Offset = this->m_stream->tellg();
		_offset += (*i)->CompressedSize;
	}

	this->m_stream->flush();
}

inline std::string __getFileName(std::string path)
{
	path = path.substr(path.find_last_of("/\\") + 1);
	return path;
}

void KOM::Kompactor::Process(vector<string> _paths) 
{
	if (_paths.size() > 0) 
	{
		tEntry* _slaveEntry = nullptr;
		for (auto i = _paths.begin(); i != _paths.end(); ++i) 
		{
			cout << (*i) << "\n";
			fstream _handle(*i, std::ios::binary | std::ios::in | std::ios::ate);
			if (!_handle.is_open())
				throw std::exception("Unable to open a file");

			_slaveEntry = new tEntry();

			std::string _fileBuffer;
			_fileBuffer.resize(_handle.tellg());
			_ASSERT(_fileBuffer.size() == _handle.tellg());
			_handle.seekg(0, std::ios_base::beg);
			_handle.read(&_fileBuffer[0], _fileBuffer.size());
			_handle.close();

			std::string _encryptedBuffer = __xor(_fileBuffer);
			_fileBuffer.clear();

			uint32_t _originalSize = _encryptedBuffer.length();
			char* _compressedBuffer = nullptr;
			uint32_t _compressedSize = 0;
			auto _zRet = zlib_compress_mem(&_encryptedBuffer[0], _encryptedBuffer.size(), &_compressedBuffer, &_compressedSize);
			if (!(_zRet == Z_OK || _zRet == _compressedSize))
				throw std::exception("Unable to compress file");

			_slaveEntry->Algorithm = 0;
			_slaveEntry->Size = _originalSize;
			_slaveEntry->CompressedSize = _compressedSize;
			_slaveEntry->FileName = __getFileName(*i);
			_slaveEntry->Checksum = "aaaaaaaa";
			_slaveEntry->FileTime = "aaaaaaaa";
			_slaveEntry->_Offset = 0;
			_slaveEntry->_Buffer.assign(_compressedBuffer, _compressedSize);

			this->m_kom->Entries.push_back(_slaveEntry);
			_slaveEntry = nullptr;
			_encryptedBuffer.clear();
		}

		return this->Process(this->m_kom->Entries);
	}
}

#include <experimental/filesystem>
void KOM::Kompactor::Process(string _root)
{
	vector<string> _childList;

	for (auto& file : experimental::filesystem::directory_iterator(_root))
		_childList.push_back(file.path().generic_string());

	this->Process(_childList);
}

#undef Z_OK