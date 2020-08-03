#pragma once
#include <exception>
#include <string>
namespace Engine::Utils
{
	class NoFileException : public std::exception
	{
	private:
		const char* file;
	public:
		NoFileException(const char* file) : file(file)
		{

		}
		const char* what() const throw()
		{
			std::string desc = "File \"";
			desc += file;
			desc += "\" not found.";
			return desc.c_str();
		}
	};
	class FileLoadException : public std::exception
	{
	private:
		const char* file;
	public:
		FileLoadException(const char* file) : file(file)
		{

		}
		const char* what() const throw()
		{
			std::string desc = "File \"";
			desc += file;
			desc += "\" could not be loaded.";
			return desc.c_str();
		}
	};
}