/*
 * ftrace_function.cpp
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <assert.h>

#include <bfd.h>
#include "execinfo.h"
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

#include <cxxabi.h>
#include <dlfcn.h>
#include <link.h>
#include <list>
#include <map>
#include <sstream>

#include "ftrace_function.hxx"

struct file_handler {
	std::string name; /* object file name */
	unsigned long addr; /* object address */
	unsigned long seg_memsz; /* segment size */
	unsigned long seg_vaddr; /* segment virtual address */
};

struct bfd_handler {
public:
	bfd * bfd_file;
	asymbol ** symbol_table;
	unsigned long nsyms;

private:
	bfd_handler() {
		bfd_file = 0;
		symbol_table = 0;
		nsyms = 0;
	}

public:

	~bfd_handler() {
		if (bfd_file != NULL)
			bfd_close(bfd_file);
		if (symbol_table != NULL)
			free(symbol_table);
	}

	static bfd_handler * new_bfd_handler(std::string const & filename) {

		int nsize = 0;
		bfd_handler * ths = new bfd_handler;
		if (ths == 0)
			goto error;

		ths->bfd_file = bfd_openr(filename.c_str(), 0);

		if (ths->bfd_file == NULL) {
			fprintf(stderr, "fail to open %s\n", filename.c_str());
			goto error;
		}

		if (!bfd_check_format(ths->bfd_file, bfd_object)) {
			fprintf(stderr, "%s is not an bfd_object\n", filename.c_str());
			goto error;
		}

		long storage_needed;

		storage_needed = bfd_get_symtab_upper_bound(ths->bfd_file);

		if (storage_needed < 0) {
			throw std::runtime_error("trace fail");
		}

		if (storage_needed == 0) {
			fprintf(stderr, "not symbol found in %s\n", filename.c_str());
			goto error;
		}

		nsize = bfd_get_symtab_upper_bound (ths->bfd_file);
		ths->symbol_table = (asymbol **) malloc(nsize);
		if (ths->symbol_table == 0)
			goto error;

		ths->nsyms = bfd_canonicalize_symtab(ths->bfd_file, ths->symbol_table);

		return ths;

		error:
		/* cleanup */
		if (ths != NULL) {
			delete ths;
		}

		return NULL;
	}

};

struct trace_data {
	static bool is_init;

	struct search_debug_info {
		int found;
		bfd_vma pc;

		char const * filename;
		char const * functionname;
		unsigned int line;

		bfd_handler * bfdh;

	};

	std::list<file_handler> files;
	std::map<unsigned long, std::string> symbols;
	std::map<std::string, bfd_handler *> bfd_opened;

	static int list_files_segments(struct dl_phdr_info *info, size_t size,
			void *_data) {

		trace_data * data = (trace_data*) _data;

		long n;
		const ElfW(Phdr) *phdr;
		phdr = info->dlpi_phdr;
		for (n = info->dlpi_phnum; --n >= 0; phdr++) {
			if (phdr->p_type == PT_LOAD) {
				file_handler x;
				x.name = info->dlpi_name;
				x.addr = info->dlpi_addr;
				x.seg_vaddr = phdr->p_paddr;
				x.seg_memsz = phdr->p_memsz;

				if (x.name.size() == 0)
					x.name = "/proc/self/exe";

				fprintf(stderr,
						"segment addr: 0x%016lx, seg_vaddr: 0x%016lx, obj: %s, seg_memsz: %lu\n",
						x.addr, x.seg_vaddr, x.name.c_str(), x.seg_memsz);
				data->files.push_back(x);
			}
		}
		return 0;
	}

	trace_data() {
		if (!is_init) {
			bfd_init();
			is_init = true;
		}

		dl_iterate_phdr(list_files_segments, this);

		std::list<file_handler>::iterator ii = files.begin();
		while (ii != files.end()) {
			if (bfd_opened.find((*ii).name) == bfd_opened.end()) {
				bfd_handler * x = bfd_handler::new_bfd_handler((*ii).name);
				if (x != 0) {
					bfd_opened[(*ii).name] = x;
				}
			}
			++ii;
		}

	}

	~trace_data() {
		std::map<std::string, bfd_handler *>::iterator i = bfd_opened.begin();
		while (i != bfd_opened.end()) {
			delete i->second;
			++i;
		}
		bfd_opened.clear();
	}

	file_handler * find_file_handler(unsigned long addr) {
		std::list<file_handler>::iterator i = files.begin();
		while (i != files.end()) {
			if ((*i).addr + (*i).seg_vaddr <= addr
					&& (*i).addr + (*i).seg_vaddr + (*i).seg_memsz > addr) {
				return &(*i);
			}
			++i;
		}
		return 0;
	}

	static void find_address_in_section(bfd *abfd, asection *section,
			void * _data) {
		search_debug_info * data = (search_debug_info *) _data;

		bfd_vma vma;
		bfd_size_type size;

		if (data->found)
			return;

		if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0)
			return;

		vma = bfd_get_section_vma(abfd, section);
		if (data->pc < vma)
			return;

		size = bfd_section_size(abfd, section);
		if (data->pc >= vma + size)
			return;

		//fprintf(stderr, "section found offset: 0x%016lx length: 0x%016lx, addr: 0x%016lx , name: %s\n", (unsigned long)bfd_get_section_vma(abfd, section), (unsigned long)bfd_section_size(abfd, section), data->pc, bfd_section_name(abfd, section));
		data->found =
				bfd_find_nearest_line(abfd, section, data->bfdh->symbol_table, data->pc - vma,
						&(data->filename), &(data->functionname), &(data->line));

		//fprintf(stderr, "xxx: %s %s %d\n", data->filename, data->functionname, data->line);
	}

	static std::string safe_demangle(char const * s) {
		int status = 0;
		char * tmp = abi::__cxa_demangle(s, 0, 0, &status);
		std::string functionname;
		if (status != 0) {
			/* demangle fail */
			functionname = s;
		} else {
			functionname = tmp;
			free(tmp);
		}
		return functionname;
	}

	bool get_symbol(void * addr, std::string & sym) {

		sym = "none";

		file_handler * fh = find_file_handler((unsigned long) addr);
		if (fh) {
			//fprintf(stderr,
			//		"segment FOUND addr: 0x%016lx, seg_vaddr: 0x%016lx, obj: %s, seg_memsz: %lu\n",
			//		fh->addr, fh->seg_vaddr, fh->name.c_str(), fh->seg_memsz);

			search_debug_info x;
			x.pc = ((unsigned long) addr) - fh->addr;
			x.found = 0;
			x.filename = 0;
			x.functionname = 0;
			x.line = 0;

			std::map<std::string, bfd_handler *>::iterator f = bfd_opened.find(
					fh->name);
			if (f != bfd_opened.end()) {
				x.bfdh = f->second;

				//fprintf(stderr, "file found %s addr: 0x%016lx seg_vaddr: 0x%016lx size = %lu \n", fh->name.c_str(), fh->addr, fh->seg_vaddr, fh->seg_memsz);
				bfd_map_over_sections(x.bfdh->bfd_file, find_address_in_section,
						&x);

				if (x.found != 0) {

					std::string functionname = safe_demangle(x.functionname);
					std::stringstream out;
					out << functionname << " at " << x.filename << ":"
							<< x.line;

					sym = out.str();
					return true;

					//fprintf(stderr, "%s %s %d\n", x.filename, x.functionname,
					//		x.line);
				}
			}
		}
		return false;
	}

};

bool trace_data::is_init = false;

void sig_handler(int sig) {
	void *array[50];
	size_t size;

	trace_data * trace = new trace_data;

	// get void*'s for all entries on the stack
	size = backtrace(array, 50);

	backtrace_symbols_fd(array, size, 2);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);

	for (unsigned int i = 0; i < size; ++i) {
		std::string sym;
		Dl_info info;
		if (trace->get_symbol(array[i], sym)) {
			fprintf(stderr, "0x%016lx %s\n", (unsigned long) array[i],
					sym.c_str());
		} else if (dladdr(array[i], &info)) {
			std::string functionname = trace_data::safe_demangle(
					info.dli_sname);
			fprintf(stderr, "0x%016lx %s in %s\n", (unsigned long) array[i],
					functionname.c_str(), info.dli_fname);
		} else {
			fprintf(stderr, "0x%016lx unknow\n", (unsigned long) array[i]);
		}
	}

	fflush(stderr);

	delete trace;
	exit(1);
}

