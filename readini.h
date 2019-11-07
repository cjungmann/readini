/**
 * line_info and parse_line_info work together to read a
 * line buffer and mark the beginning and end of its
 * constituent parts, the tag and value.
 *
 * This information is used by **read_section** to allocate
 * memory to populate the **iniline** structure.
 */
struct line_info
{
   const char *tag;
   const char *value;
   int len_tag;
   int len_value;
};

int parse_line_info(const char *buffer, struct line_info *li);


struct iniline
{
   const char *tag;
   const char *value;
   struct iniline *next;
};

struct inisection
{
   const char *section_name;
   const struct iniline *lines;
   struct inisection *next;
};

typedef void (*FileUser)(int fh);
typedef void (*LineUser)(const char *section_name, const char *tag_name, const char *value);
typedef void (*SectionUser)(int fh, const char *section_name, LineUser lu);
typedef void (*InilineUser)(const struct iniline *lines);
typedef void (*SectionReader)(int fh, const char *section_name, InilineUser iu);
typedef void (*IniFileUser)(const struct inisection *section);

struct read_inifile_bundle
{
   char *buffer;
   struct inisection *head;
   struct inisection *tail;
   IniFileUser ifu;
   int fh;
};


/**
 * Internal functions, supporting public functions further down.
 */

void read_inifile_section_lines(struct read_inifile_bundle* bundle);
void read_inifile_section_recursive(struct read_inifile_bundle* bundle);
void read_inifile(int fh, IniFileUser ifu);



/**
 * Public functions, intended for use by the library user.
 */

void cb_open(const char *path, FileUser fu);
int seek_section(int fh, const char* section_name, SectionUser su, LineUser lu);
void read_section(int fh, const char *section_name, InilineUser iu);

void get_inifile(const char *filepath, IniFileUser ifu);


const char* find_value(const struct iniline *il, const char *tag);
