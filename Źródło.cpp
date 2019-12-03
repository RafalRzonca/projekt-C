/*
#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
//#include <filesystem>
#include <experimental/filesystem>
#include <Windows.h>
#include<string.h>

namespace fs = std::experimental::filesystem;
using namespace std::chrono_literals;

void main()
{
	fs::path p = fs::current_path() / "Nowy dokument tekstowy.txt";
	std::cout << p << std::endl;
	//std::ofstream(p.c_str()).put('a'); // create file
	auto ftime = fs::last_write_time(p);

	// assuming system_clock for this demo
	// note: not true on MSVC; C++20 will allow portable output
	std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
	std::cout << "File write time is " << std::asctime(std::localtime(&cftime)) << std::endl;
	auto pom1= fs::last_write_time(p);

	fs::last_write_time(p, ftime + 1h); // move file write time 1 hour to the future
	ftime = fs::last_write_time(p); // read back from the filesystem

	if (pom1 > fs::last_write_time(p)) {
		std::time_t cftime = decltype(pom1)::clock::to_time_t(ftime);
		std::cout << "File write time is " << std::asctime(std::localtime(&cftime)) << std::endl;
	}
	else {
		std::cout << "File write time is " << std::asctime(std::localtime(&cftime)) << std::endl;
		std::time_t cftime = decltype(pom1)::clock::to_time_t(ftime);
	}

	fs::last_write_time(p, ftime + 10h); // move file write time 1 hour to the future
	ftime = fs::last_write_time(p); // read back from the filesystem

	cftime = decltype(ftime)::clock::to_time_t(ftime);
	std::cout << "File write time is " << std::asctime(std::localtime(&cftime)) << std::endl;
	//fs::remove(p);

	//system("dir /T:C ród³o.cpp");
	
	system("pause");
}
*/

// xtree.cpp : Defines the entry point for the console application.
//
#include <stdio.h> 
#include <stdlib.h> 
#include <windows.h> 
#include <string>
#include <iostream>
#include <set>

void errormessage(void)
{
	LPVOID  lpMsgBuf;
	FormatMessage
	(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // Default language
		(LPTSTR)& lpMsgBuf,
		0,
		NULL
	);
	fprintf(stderr, "%s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void format_time(FILETIME *t, std::string& ret)
{
	char *buff;
	FILETIME    localtime;
	SYSTEMTIME  sysloc_time;
	FileTimeToLocalFileTime(t, &localtime);
	FileTimeToSystemTime(&localtime, &sysloc_time);
	sprintf
	(
		buff,
		"%4d/%02d/%02d %02d:%02d:%02d",
		sysloc_time.wYear,
		sysloc_time.wMonth,
		sysloc_time.wDay,
		sysloc_time.wHour,
		sysloc_time.wMinute,
		sysloc_time.wSecond
	);
	ret += buff;
}

void format_attr(uint64_t attr, std::string& buff)
{
	buff += (attr & FILE_ATTRIBUTE_ARCHIVE ? 'A' : '-');
	buff += (attr & FILE_ATTRIBUTE_SYSTEM ? 'S' : '-');
	buff += (attr & FILE_ATTRIBUTE_HIDDEN ? 'H' : '-');
	buff += (attr & FILE_ATTRIBUTE_READONLY ? 'R' : '-');
	buff += (attr & FILE_ATTRIBUTE_DIRECTORY ? 'D' : '-');
	buff += (attr & FILE_ATTRIBUTE_ENCRYPTED ? 'E' : '-');
	buff += (attr & FILE_ATTRIBUTE_COMPRESSED ? 'C' : '-');
}

struct flist
{
	int             num_entries;
	int             max_entries;
	WIN32_FIND_DATA *files;
};

//void addfile(flist *list, WIN32_FIND_DATA data)
//{
//	if (list->num_entries == list->max_entries)
//	{
//		int             newsize = list->max_entries == 0 ? 16 : list->max_entries * 2;
//		WIN32_FIND_DATA *temp = (WIN32_FIND_DATA *)realloc(list->files, newsize * sizeof(WIN32_FIND_DATA));
//		if (temp == NULL)
//		{
//			fprintf(stderr, "Out of memory\n");
//			exit(1);
//		}
//		else
//		{
//			list->max_entries = newsize;
//			list->files = temp;
//		}
//	}
//
//	list->files[list->num_entries++] = data;
//}

#define THROW( msg, exc ) try{ throw exc(msg); }catch(const exc&){ throw; }

struct time_type
{
	using T = uint64_t;

	uint64_t timestamp;

	time_type() = default;

	explicit time_type(const FILETIME& src)
	{
		this->operator=(src);		
	}

	time_type& operator=(const FILETIME& src)
	{
		union
		{
			FILETIME f;
			T t;
		} u;
		u.f = src;
		timestamp = u.t;
		return *this;
	}
};

bool operator<(const time_type& src1, const time_type& src2)
{
	return src1.timestamp < src2.timestamp;
}

struct find_result_item
{
	//using time_type = unsigned long long int;
	using size_type = unsigned long long int;
	using str = std::string;

	int64_t attribiutes;
	time_type creation;
	time_type access;
	time_type write;
	size_type file_size;
	size_type reserved;
	str file_name;
	str alt_file_name;

	find_result_item(WIN32_FIND_DATA* src)
	{
		if (src == nullptr) THROW("gdzie kurwa z nullptr", std::invalid_argument);
		attribiutes = src->dwFileAttributes;
		creation = src->ftCreationTime;
		access = src->ftLastAccessTime;
		write = src->ftLastWriteTime;
		union
		{
			struct
			{
				uint32_t a, b;
			};
			uint64_t res;
		} conv;
		conv.a = src->nFileSizeLow;
		conv.b = src->nFileSizeHigh;
		file_size = conv.res;
		conv.a = src->dwReserved0;
		conv.b = src->dwReserved1;
		reserved = conv.res;
		file_name = src->cFileName;
		alt_file_name = src->cAlternateFileName;
	}

};

int sortfiles(const void *a, const void *b)
{
	const WIN32_FIND_DATA *pa = (WIN32_FIND_DATA *)a;
	const WIN32_FIND_DATA *pb = (WIN32_FIND_DATA *)b;
	int                   a_is_dir = pa->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	int                   b_is_dir = pb->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	if (a_is_dir ^ b_is_dir)
	{
		// one of each, prefer the directories first
		if (a_is_dir) return(-1);
		if (b_is_dir) return(+1);
		return(0);
	}
	else
	{
		// both files, or both directories - return the strcmp result
		return(strcmp(pa->cFileName, pb->cFileName));
	}
}

#include <vector>
#include <algorithm>

void doit(const std::string& name)
{
	//flist           list = { 0, 0, NULL };
	std::vector<find_result_item> list;
	HANDLE          h;
	WIN32_FIND_DATA info;
	int             i;

	// build a list of files
	std::string pattern(name);
	pattern.append("\\*");
	h = FindFirstFile(pattern.c_str(), &info);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(strcmp(info.cFileName, ".") == 0 || strcmp(info.cFileName, "..") == 0))
			{
				addfile(&list, info);
			}
		} while (FindNextFile(h, &info));
		if (GetLastError() != ERROR_NO_MORE_FILES) errormessage();
		FindClose(h);
	}
	else
	{
		errormessage();
	}

	auto sort_by_name = [](const find_result_item& it1, const find_result_item it2)
	{
		return it1.file_name < it2.file_name;
	};

	// sort them
	//qsort(list.files, list.num_entries, sizeof(list.files[0]), sort_by_name);
	std::sort(list.begin(), list.end(), sort_by_name);

	// print out in sorted order
	int numdirs = 0;
	{
		char  t1[50], t2[50], t3[50], a[10];
		format_time(&list.files[i].ftCreationTime, t1);
		format_time(&list.files[i].ftLastAccessTime, t2);
		format_time(&list.files[i].ftLastWriteTime, t3);
		format_attr(list.files[i].dwFileAttributes, a);
		if (list.files[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// 'null' date for directory access times, which change each time
			// we run this tool
			sprintf(t2, "%4d/%02d/%02d %02d:%02d:%02d", 2000, 1, 1, 0, 0, 0);
		}

		printf("%s %10ld %s %s %s %s\\%s\n", a, list.files[i].nFileSizeLow, t1, t2, t3, "", list.files[i].cFileName);//%s\\%s
		if (list.files[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) numdirs++;
	}

	// now process all the sub-dirs
	// free all the files first, to save a bit of space
	// the sort function will have put them all at the end.

	list.files = (WIN32_FIND_DATA *)realloc(list.files, numdirs * sizeof(WIN32_FIND_DATA));
	for (i = 0; i < numdirs; i++)
	{
		//char  newroot[MAX_PATH];
		//sprintf(newroot, "%s\\%s","*.*", list.files[i].cFileName);
		//sprintf(newroot, "",list.files[i].cFileName);
		SetCurrentDirectory(list.files[i].cFileName);
		//doit(newroot);
		SetCurrentDirectory("..");
	}

	// free the remainder
	free(list.files);
}

void banner(void)
{
	//       12345678901234567890
	printf("Attribs ");
	printf("      Size ");
	printf("       CreationTime ");
	printf("     LastAccessTime ");
	printf("      LastWriteTime ");
	printf("Filename\n");
}

int main(int argc, char *argv[])
{
	char  olddir[MAX_PATH];
	std::string sciezka;
	printf("Podaj sciezke do katalogu: ");
	std::cin >> sciezka;
	std::cout << std::endl;
	if(argc>1)
	{
	if (GetCurrentDirectory(MAX_PATH, olddir) == 0)
	{
		errormessage();
		exit(1);
	}

	if (!SetCurrentDirectory(argv[1]))
	{
		errormessage();
		exit(1);
	}

	banner();
	doit(olddir);
	SetCurrentDirectory(olddir);
}
	else
	{
		banner();
		doit(sciezka);
	}

	system("pause");
	return(0);
}