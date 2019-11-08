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


