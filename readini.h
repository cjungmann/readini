
/**
 * Structure for node of linked list of line contents.
 */
typedef struct ri_line
{
   const char *tag;
   const char *value;
   struct ri_line *next;
} ri_Line;

/**
 * Structore for node of linked list of sections info.
 */
typedef struct ri_section
{
   const char *section_name;
   const struct ri_line *lines;
   struct ri_section *next;
} ri_Section;

/**
 * Callback function pointer typedefs for *ri_open_section()* and *ri_open_file()*
 */
typedef void (*ri_File_User)(int fh);
typedef void (*ri_Lines_Browser)(const ri_Line *lines);
typedef void (*ri_Sections_Browser)(const ri_Section *section);

void ri_open(const char *path, ri_File_User cb_file_user);
void ri_open_section(int fh, const char *section_name, ri_Lines_Browser cb_lines_browser);
void ri_open_file(const char *filepath, ri_Sections_Browser ifu);

const char* ri_find_value(const ri_Line* lines_head,
                          const char* tag_name);

const char* ri_find_section_value(const ri_Section* sections_head,
                                  const char* section_name,
                                  const char* tag_name);
