#include <stdio.h>

#define MAX_CLINE 200

/**
 * Include files for open(), read(), etc.
 * See **man** 3 open
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>  // for lseek() 
#include <errno.h>
#include <string.h>  // for strlen(), etc;

#include <ctype.h>   // for isspace
#include <alloca.h>  // for alloca
#include <assert.h>

#include "readini.h"
#include "readini_private.h"

/** @brief A more restrictive reckoning of what constitutes a space. */
int is_space(const char *val)     { return strchr(" \t", *val) != NULL; }

/** @brief Identifies a character that cannot be in a tag, thus marking the tag's end. */
int is_end_tag(const char *val)   { return strchr("#:= \t", *val) != NULL; }

/** @brief Returns TRUE if first character of trimmed line buffer is '['. */
int line_is_section_type(const char *buffer)
{
   return buffer[0] == '[';
}

/** @brief Reports if '[' and ']' in buffer, without regard to what's between the square braces. */
int line_contains_section_head(const char *buffer)
{
   const char *ptr = "";
   if (line_is_section_type(buffer))
   {
      ptr = buffer;
      while (*++ptr && *ptr != ']')
         ;
   }

   return *ptr == ']';
}

/** @brief Reports if buffer contains the *section_name* specified section header. */
int line_is_section(const char *buffer, const char *section_name)
{
   int len_name = strlen(section_name);
   if (line_is_section_type(buffer))
      return len_name+1 < MAX_CLINE
         && buffer[len_name+1] == ']'
         && 0 == strncmp(&buffer[1], section_name, len_name);
   else
      return 0;
}

/**
 * @brief Read character at a time to first newline or EOF.
 */
void discard_file_chars_to_newline(int fh)
{
   char minibuff;
   ssize_t bytes_read;

   while( (bytes_read = read(fh, &minibuff, 1)) )
   {
      if (minibuff == '\n')
         break;
   }
}

/**
 * @brief Scans the buffer to find a tag and value.
 *
 * The line_info parameter is set with pointers to the
 * to where the tag and value substrings begin, and
 * length members with the length of the tag and value
 * substrings.
 */
int ri_parse_line_info(const char *buffer, struct ri_line_info *li)
{
   const char *ptr;

   memset(li, 0, sizeof(struct ri_line_info));

   if (*buffer == '\0')
      return 0;

   ptr = buffer;

   // Find first non-tag character
   while (*++ptr && !is_end_tag(ptr))
      ;

   li->tag = buffer;
   li->len_tag = ptr - buffer;

   if (*ptr)
   {
      // move past spaces and/or operators to find value:
      while (*++ptr && is_end_tag(ptr))
         ;

      li->value = ptr;

      // Find the end of the value
      while (*++ptr)
         ;

      // Back-off ending spaces
      if (ptr > li->value)
      {
         while (ptr > li->value && is_space(ptr))
            --ptr;

         li->len_value = ptr - li->value + 1;
      }
      else
      {
         li->value = NULL;
         li->len_value = 0;
      }
   }
   return 1;
}

/**
 * @brief Read a line from an open file handle.
 *
 * @return TRUE until EOF
 *
 * This function reads characters until it reaches a newline or the EOF.
 * The buffer will contain as many of the read characters as fit, with 
 * the following conditions:
 * 
 * - The buffer string will be terminated with a '\0'.
 * - A comment, introduced by '#', will not be included in the buffer.
 * - An escaped '#' i.e. "\#" will be rendered as a single '#' in the tag,
 *   without being treated as an introduction to a comment.
 * - A too-long line will be truncated
 * - The function returns TRUE until the call after it reaches the EOF.
 * - With each return of **read_line**, the file pointer is at the
 *   beginning of a text line.
 */
int read_line(int fh, char *buffer, int buff_len)
{
   char *ptr = buffer;
   char *end = buffer + buff_len;
   ssize_t bytes_read;

   *ptr = '\0';

   while (ptr < end)
   {
      bytes_read = read(fh, ptr, 1);

      if (bytes_read==0)
      {
         if (ptr > buffer)
            // fake a newline if we've read some characters
            *ptr = '\n';
         else
            // If EOF with no contents, we're done
            return 0;
      }

      // If a comment is starting, pretend it's the EOL
      if (*ptr == '#')
      {
         if (ptr > buffer && *(ptr-1) == '\\')
         {
            *(ptr-1) = '#';
            continue;
         }
         else
         {
            *ptr = '\n';
            discard_file_chars_to_newline(fh);
         }
      }

      if (*ptr == '\n')
         break;
      // Ignore leading spaces:
      else if ( is_space(ptr) && ptr == buffer)
         continue;
      else
         ++ptr;
   }

   // Reached end without finding newline.
   if (ptr >= end)
   {
      discard_file_chars_to_newline(fh);
      *--ptr = '\n';
   }

   if (*ptr == '\n')
      *ptr = '\0';

   return 1;
}

/**
 * @brief Position the file pointer to the line following a named section head.
 *
 * @return TRUE if the section was found, otherwise FALSE (0).
 *
 * A named section head is a string enclosed in square-brackets "[]".
 * If the function returns TRUE, the calling function can expect
 * the file pointer to be at the first content line of the section.
 */
int find_section(int fh, const char* section_name)
{
   char buffer[MAX_CLINE];
   int len_name = strlen(section_name);
   int found = 0;

   // Move to top of the file:
   lseek(fh, SEEK_SET, 0);

   while (read_line(fh, buffer, MAX_CLINE))
   {
      if (buffer[0] == '['
          && strncmp(&buffer[1], section_name, len_name)==0
          && buffer[len_name+1] == ']')
      {
         found = 1;
         break;
      }
   }

   return found;
}

/**
 * @brief Works with read_inifile_section_recursive to collect configuration data.
 */
void read_inifile_section_lines(struct read_inifile_bundle* bundle)
{
   struct ri_line_info li;
   struct ri_line *new_iniline, *root = NULL, *tail = NULL;
   char *tag, *value;
   
   char *buffer = bundle->buffer;


   while (read_line(bundle->fh, buffer, MAX_CLINE))
   {
      memset(&li, 0, sizeof(li));

      if (line_is_section_type(buffer))
      {
         // Prevent callback-triggering while-loop exit
         // by returning directly at return from recursion:
         return read_inifile_section_recursive(bundle);
      }
      else if ( ri_parse_line_info(buffer, &li) )
      {
         new_iniline = (struct ri_line *)alloca(sizeof(struct ri_line));
         memset(new_iniline, 0, sizeof(struct ri_line));

         // Set empty tail members to tag/value of the line
         tag = (char *)alloca(li.len_tag+1);
         memcpy(tag, li.tag, li.len_tag);
         tag[li.len_tag] = '\0';
         new_iniline->tag = tag;

         if (li.len_value)
         {
            value = (char*)alloca(li.len_value+1);
            memcpy(value, li.value, li.len_value);
            value[li.len_value] = '\0';

            new_iniline->value = value;
         }

         if (tail)
         {
            tail->next = new_iniline;
            tail = new_iniline;
         }
         else
            bundle->tail->lines = root = tail = new_iniline;
      }
   }

   // Despite the recursion, we should only arrive here once,
   // when the configuration file has been completely read.
   // We'll close the file handle before invoking the callback
   // to preserve system resources.
   //
   // Further, we close the file and set the file descriptor
   // in *bundle* to -1 so it won't be closed twice.
   close(bundle->fh);
   bundle->fh = -1;

   // Make linked data available to requesting function
   (*bundle->ifu)(bundle->head);
}

/**
 * When entering this function, the bundle->buffer must contain
 * the section line, so the it can be called for the next section
 * header without having to rewind the file.
 */
void read_inifile_section_recursive(struct read_inifile_bundle* bundle)
{
   assert(bundle->buffer[0] == '[');

   char *ptr;
   struct ri_section* new_inisection = NULL;
   char *buffer = bundle->buffer;
   char *section_name = NULL;
   int section_name_length = 0;

   // Find trailing ']' to delimit section name.
   // Initial increment to skip the leading '[',
   ptr = buffer;
   while (*++ptr && *ptr != ']')
      ;

   if (*ptr)
   {
      section_name_length = ptr - buffer - 1;
      section_name = (char*)alloca(section_name_length+1);
      memcpy(section_name, &buffer[1], section_name_length);
      section_name[section_name_length] = '\0';
   }

   if (section_name)
   {
      new_inisection = (struct ri_section*)alloca(sizeof(struct ri_section));
      memset(new_inisection, 0, sizeof(struct ri_section));
      new_inisection->section_name = section_name;

      if (bundle->tail)
      {
         bundle->tail->next = new_inisection;
         bundle->tail = new_inisection;
      }
      else
         bundle->tail = bundle->head = new_inisection;

      read_inifile_section_lines(bundle);
   }
}

/**
 * @brief Opens a file, invokes callback with file descriptor, then closes file.
 *
 * A simple implementation of my model of resource cleanup: call this function
 * with a callback argument that is invoked when the resources are available.
 * When the callback function returns, the resources are unwound.
 *
 * @param[in] path         Path to configuration file.
 * @param[in] cb_file_user Function pointer called with an open file descriptor.
 * @param[in] data         Castable void pointer to custom application data.
 */
void ri_open(const char *path, ri_File_User cb_file_user, void* data)
{
   int fh = open(path, O_RDONLY);
   if (fh == -1)
   {
      fprintf(stderr, "Failed to open \"%s\".", path);
   }
   else
   {
      (*cb_file_user)(fh, data);
      close(fh);
   }
}

/**
 * @brief Primary interface for acquiring the contents of a section.
 *
 * This function is used with an open file descriptor to retrieve
 * the lines of a specific section.  It works with a file descriptor
 * to support loading multiple sections without closing and opening
 * the file between section readings.

 * @param fh               File descriptor of an open file.
 * @param section_name     Name of section to retrieve.
 * @param cb_lines_browser Callback function that will be called with a
 *                         pointer to the head of a **ri_line** linked list.
 * @param[in] data         Castable void pointer to custom application data.
 */
void ri_open_section(int fh, const char *section_name, ri_Lines_Browser cb_lines_browser, void* data)
{
   off_t saved_offset = lseek(fh, SEEK_CUR, 0);

   char buffer[MAX_CLINE];

   struct ri_line_info li;
   struct ri_line *new_iniline, *root = NULL, *tail =NULL;

   char *tag, *value;

   if (find_section(fh, section_name))
   {
      while (read_line(fh, buffer, MAX_CLINE))
      {
         if (line_is_section_type(buffer))
            break;
         else if ( ri_parse_line_info(buffer, &li) )
         {
            new_iniline = (struct ri_line *)alloca(sizeof(struct ri_line));
            memset(new_iniline, 0, sizeof(struct ri_line));

            // Set empty tail members to tag/value of the line
            tag = (char *)alloca(li.len_tag+1);
            memcpy(tag, li.tag, li.len_tag);
            tag[li.len_tag] = '\0';
            new_iniline->tag = tag;

            if (li.len_value)
            {
               value = (char*)alloca(li.len_value+1);
               memcpy(value, li.value, li.len_value);
               value[li.len_value] = '\0';

               new_iniline->value = value;
            }

            if (tail)
            {
               tail->next = new_iniline;
               tail = new_iniline;
            }
            else
               root = tail = new_iniline;
         }
      };
   }

   (*cb_lines_browser)(fh, root, data);
   
   lseek(fh, SEEK_SET, saved_offset);
}

/**
 * @brief Return value string associated with a tag name.
 *
 * Scans a linked list of lines info for a line whose tag
 * value matches the *tag_name* parameter.
 *
 * @param lines_head Pointer to first node of a linked list
 *                   of lines info.
 * @param tag_name   Name of tag to match in the linked lines
 *                   list.
 *
 * @return Pointer to value string if tag is found and it
 *         has a value.  Nonexistent tags and empty found
 *         tags will return NULL;
 */
const char* ri_find_value(const struct ri_line* lines_head,
                          const char* tag_name)
{
   const struct ri_line *ptr = lines_head;

   while (ptr)
   {
      if (0 == strcmp(ptr->tag, tag_name))
         return ptr->value;

      ptr = ptr->next;
   }

   return NULL;
}

/**
 * @brief Reads an entire configuration file into a linked list.
 *
 * Opens and reads the configuration file into a linked list of
 * sections, leaving out the comments and empty lines.
 *
 * @param filepath            Path to the configuration file.
 * @param cb_sections_browser Pointer to function that will consume the
 *                            sections linked list.  **Do not** quote
 *                            the function name.  See example below.
 *
 * @code
 * void section_browser(const ri_Section* sections)
 * {
 *    const char* user = ri_find_section_value(sections, "bogus", "user");
 *    const char* pword = ri_find_section_value(sections, "bogus", "password");
 * }
 *
 * int main(int argc, char** argv)
 * {
 *    // Pass function pointer by naming the function.
 *    // Do not quote the function name.  You want its address.
 *    ri_open_file("~/.mymail.conf", section_browser);
 * }
 * @endcode
 *
 */
void ri_read_file(const char *filepath, ri_Sections_Browser cb_sections_browser)
{
   char buffer[MAX_CLINE];
   struct read_inifile_bundle bundle;

   int fh = open(filepath, O_RDONLY);
   if (fh == -1)
   {
      fprintf(stderr, "Failed to open \"%s\".", filepath);
   }
   else
   {
      // Initialize the bundle
      memset(&bundle, 0, sizeof(struct read_inifile_bundle));
      bundle.fh = fh;
      bundle.buffer = buffer;
      bundle.ifu = cb_sections_browser;

      // Read lines until the first section, beginning work if one is found
      while (read_line(fh, buffer, MAX_CLINE))
      {
         if (line_contains_section_head(buffer))
         {
            read_inifile_section_recursive(&bundle);
            break;
         };
      }

      // Close file if not already closed:
      if (bundle.fh != -1)
         close(bundle.fh);
   }
}

/**
 * @brief Return value string associated with a tag name in the
 *        named section.
 *
 * Scans the linked list of sections (provided by a call to
 * the *ri_open_file* function) for a section whose name
 * matches the *section_name* parameter, then scans the lines
 * in a matched section for a line whose tag value matches
 * the *tag_name* parameter.
 *
 * @param lines_head Pointer to first node of a linked list
 *                   of lines info.
 * @param tag_name   Name of tag to match in the linked lines
 *                   list.
 *
 * @return Pointer to value string if tag is found and it
 *         has a value.  Nonexistent tags and empty found
 *         tags will return NULL;
 */
const char* ri_find_section_value(const ri_Section* sections_head,
                                  const char* section_name,
                                  const char* tag_name)
{
   const ri_Line* lptr;
   const ri_Section* sptr = sections_head;
   while (sptr)
   {
      if (0 == strcmp(sptr->section_name, section_name))
      {
         lptr = sptr->lines;
         while (lptr)
         {
            if (0 == strcmp(lptr->tag, tag_name))
               return lptr->value;

            lptr = lptr->next;
         }
      }

      sptr = sptr->next;
   }

   return NULL;
}
