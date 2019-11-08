/**
 * ir_line_info and parse_line_info work together to read a
 * line buffer and mark the beginning and end of its
 * constituent parts, the tag and value.
 *
 * This information is used to allocate memory to populate
 * the **ri_line** structure.
 */
struct ri_line_info
{
   const char *tag;
   const char *value;
   int len_tag;
   int len_value;
};

int ri_parse_line_info(const char *buffer, struct ri_line_info *li);



/** Public Structures, returned by ri_read_section and ri_read_file  */

typedef struct ri_line
{
   const char *tag;
   const char *value;
   struct ri_line *next;
} ri_Line;

typedef struct ri_section
{
   const char *section_name;
   const struct ri_line *lines;
   struct ri_section *next;
} ri_Section;


/**
 * Public use callback function pointer typedefs for *ri_open_section()* and *ri_open_file()*
 */
typedef void (*ri_File_User)(int fh);
typedef void (*ri_Lines_Browser)(const ri_Line *lines);
typedef void (*ri_Sections_Browser)(const ri_Section *section);

/**
 * Internal structure used to collect data from configuration file.
 */
typedef struct read_inifile_bundle
{
   char *buffer;
   ri_Section *head;
   ri_Section *tail;
   ri_Sections_Browser ifu;
   int fh;
} Bundle;


/**
 * Internal functions, supporting public functions further down.
 */

void read_inifile_section_lines(struct read_inifile_bundle* bundle);
void read_inifile_section_recursive(struct read_inifile_bundle* bundle);



/**
 * Public functions, intended for use by the library user.
 */

void ri_open(const char *path, ri_File_User cb_file_user);
void ri_open_section(int fh, const char *section_name, ri_Lines_Browser cb_lines_browser);
void ri_open_file(const char *filepath, ri_Sections_Browser ifu);

const char* ri_find_value(const ri_Line* lines_head,
                          const char* tag_name);

const char* ri_find_section_value(const ri_Section* sections_head,
                                  const char* section_name,
                                  const char* tag_name);
