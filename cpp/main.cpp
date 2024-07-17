#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

struct FileItem {
	std::string name;
	size_t bytes = 0;
	size_t blocks = 0;
	bool isdir = false;
	bool islnk = false;
};

void traverse(std::string const &dir, std::vector<FileItem> *list, int depth)
{
	DIR *d;
	d = opendir(dir.c_str());
	if (!d) {
		perror(dir.c_str());
		return;
	}
	struct dirent *e;
	while (1) {
		e = readdir(d);
		if (!e) break;
		char const *name = e->d_name;
		if (strcmp(name, ".") == 0) continue;
		if (strcmp(name, "..") == 0) continue;
		struct stat st;
		if (lstat(((dir + '/' + name).c_str()), &st) == 0) {
			FileItem item;
			item.isdir = S_ISDIR(st.st_mode);
			item.islnk = S_ISLNK(st.st_mode);
			item.name = name;
			item.bytes = st.st_size;
			item.blocks = st.st_blocks;
			list->push_back(item);
		}
	}
	closedir(d);

	for (FileItem &item: *list) {
		std::string path = dir + '/' + item.name;
		if (!item.islnk && item.isdir && depth < 100) {
			std::vector<FileItem> sublist;
			traverse(path.c_str(), &sublist, depth + 1);
			for (FileItem const &subitem : sublist) {
				item.bytes += subitem.bytes;
				item.blocks += subitem.blocks;
			}
		}
	}
}

void used(std::string const &dir)
{
	std::vector<FileItem> list;

	traverse(dir, &list, 0);

	std::sort(list.begin(), list.end(), [&](FileItem const &left, FileItem const &right){
		auto Compare = [](FileItem const &left, FileItem const &right){
			if (left.blocks < right.blocks) return -1;
			if (left.blocks > right.blocks) return 1;
			if (left.name < right.name) return -1;
			if (left.name > right.name) return 1;
			return 0;
		};
		return Compare(left, right) > 0;
	});

	size_t indent = 0;
	for (FileItem &item: list) {
		char buf[PATH_MAX + 5];
		std::string path;
		if (item.islnk) {
			strcpy(buf, " -> ");
			path = dir + '/' + item.name;
			ssize_t len = readlink(path.c_str(), buf + 4, PATH_MAX);
			if (len < 0) {
				len = 0;
			}
			buf[4 + len] = 0;
			path = item.name + buf;
			buf[0] = 0;
		} else {
			sprintf(buf, "%u", item.blocks / 2);
			if (indent == 0) {
				indent = strlen(buf);
			}
			path = item.name;
			if (item.isdir) {
				path += '/';
			}
		}
		printf("% *s  %s\n", (int)indent, buf, path.c_str());
	}
}

int main(int argc, char *argv[])
{
	const char *path = (argc > 1) ? argv[1] : ".";
	used(path);
	return 0;
}
