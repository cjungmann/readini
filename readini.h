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

typedef void (*FileUser)(int fh);
typedef void (*LineUser)(const char *section_name, const char *tag_name, const char *value);
typedef void (*SectionUser)(int fh, const char *section_name, LineUser lu);
typedef void (*InilineUser)(const struct iniline *lines);
typedef void (*SectionReader)(int fh, const char *section_name, InilineUser iu);

void cb_open(const char *path, FileUser fu);
int seek_section(int fh, const char* section_name, SectionUser su, LineUser lu);
void read_section(int fh, const char *section_name, InilineUser iu);

