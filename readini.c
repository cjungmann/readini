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

/** @brief A more restrictive reckoning of what constitutes a space. */
int is_space(const char *val)     { return strchr(" \t", *val) != NULL; }

/** @brief Identifies a character that cannot be in a tag, thus marking the tag's end. */
int is_end_tag(const char *val)   { return strchr("#:= \t", *val) != NULL; }

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
 * @brief Opens a file, invokes callback with file descriptor, then closes file.
 *
 * A simple implementation of my model of resource cleanup: call this function
 * with a callback argument that is invoked when the resources are available.
 * When the callback function returns, the resources are unwound.
 */
void cb_open(const char *path, FileUser fu)
{
   int fh = open(path, O_RDONLY);
   if (fh == -1)
   {
      fprintf(stderr, "Failed to open \"%s\".", path);
   }
   else
   {
      (*fu)(fh);
      close(fh);
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
int parse_line_info(const char *buffer, struct line_info *li)
{
   const char *ptr;

   memset(li, 0, sizeof(struct line_info));

   if (*buffer == '\0')
      return 0;

   ptr = buffer;

   // Find first non-tag character
   while (*++ptr && !is_end_tag(ptr))
      ;

   li->tag = buffer;
   li->len_tag = ptr - buffer;

   // move past spaces and/or operators to find value:
   while (*++ptr && is_end_tag(ptr))
      ;

   if (*ptr)
   {
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

/** @brief Returns TRUE if first character of trimmed line buffer is '['. */
int line_is_section_type(const char *buffer)
{
   return buffer[0] == '[';
}


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

/** @brief Returns TRUE if the line buffer begins with [$section_name]. */
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
 * @brief Function for alternate processing of a section, with two callback parameters.
 *
 * Use this function to process a section's lines individually rather than
 * as a group with **read_section**.
 */
int seek_section(int fh, const char* section_name, SectionUser su, LineUser lu)
{
   off_t saved_offset = lseek(fh, SEEK_CUR, 0);
   char buffer[MAX_CLINE];
   int found = 0;

   if (saved_offset != -1)
   {
      lseek(fh, SEEK_SET, 0);

      while (read_line(fh, buffer, MAX_CLINE))
      {
         if (line_is_section(buffer, section_name))
         {
            (*su)(fh, section_name, lu);
            found = 1;
            break;
         }
      }

      lseek(fh, SEEK_SET, saved_offset);
   }

   return found;
}

/**
 * @brief Primary interface for acquiring the contents of a section.
 */
void read_section(int fh, const char *section_name, InilineUser iu)
{
   off_t saved_offset = lseek(fh, SEEK_CUR, 0);

   char buffer[MAX_CLINE];

   struct line_info li;
   struct iniline *new_iniline, *root = NULL, *tail =NULL;

   char *tag, *value;

   if (find_section(fh, section_name))
   {
      while (read_line(fh, buffer, MAX_CLINE))
      {
         if (line_is_section_type(buffer))
            break;
         else if ( parse_line_info(buffer, &li) )
         {
            new_iniline = (struct iniline *)alloca(sizeof(struct iniline));
            memset(new_iniline, 0, sizeof(struct iniline));

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

   (*iu)(root);
   
   lseek(fh, SEEK_SET, saved_offset);
}





/**
 * @brief Works with read_inifile_section_recursive to collect configuration data.
 */
void read_inifile_section_lines(struct read_inifile_bundle* bundle)
{
   struct line_info li;
   struct iniline *new_iniline, *root = NULL, *tail = NULL;
   char *tag, *value;
   
   char *buffer = bundle->buffer;

   while (read_line(bundle->fh, buffer, MAX_CLINE))
   {
      if (line_is_section_type(buffer))
      {
         // Prevent callback-triggering while-loop exit
         // by returning directly at return from recursion:
         return read_inifile_section_recursive(bundle);
      }
      else if ( parse_line_info(buffer, &li) )
      {
         new_iniline = (struct iniline *)alloca(sizeof(struct iniline));
         memset(new_iniline, 0, sizeof(struct iniline));

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

   // This loop should only terminate
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
   struct inisection* new_inisection = NULL;
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
      new_inisection = (struct inisection*)alloca(sizeof(struct inisection));
      memset(new_inisection, 0, sizeof(struct inisection));
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

void read_inifile(int fh, IniFileUser ifu)
{
   char buffer[MAX_CLINE];

   // Initialize the bundle
   struct read_inifile_bundle bundle;
   memset(&bundle, 0, sizeof(struct read_inifile_bundle));
   bundle.fh = fh;
   bundle.buffer = buffer;
   bundle.ifu = ifu;

   // Find the first section header:
   while (read_line(fh, buffer, MAX_CLINE))
   {
      if (line_contains_section_head(buffer))
      {
         read_inifile_section_recursive(&bundle);
         break;
      };
   }
}

void get_inifile(const char *filepath, IniFileUser ifu)
{
   int fh = open(filepath, O_RDONLY);
   if (fh == -1)
   {
      fprintf(stderr, "Failed to open \"%s\".", filepath);
   }
   else
   {
      read_inifile(fh, ifu);
      close(fh);
   }
}


/**
 * @brief Return value string associated with a tag name.
 *
 * @return Pointer to value string if tag is found and it
 *         has a value.  Nonexistent tags and empty found
 *         tags will return NULL;
 */
const char* find_value(const struct iniline *il, const char *tag)
{
   const struct iniline *ptr = il;

   while (ptr)
   {
      if (0 == strcmp(tag, ptr->tag))
         return ptr->value;

      ptr = ptr->next;
   }

   return NULL;
}
