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

int is_space(const char *val)     { return strchr(" \t", *val) != NULL; }
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
 * - A too-long line will be truncated
 * - A zero-length string will return FALSE.
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

int line_is_section_type(const char *buffer)
{
   return buffer[0] == '[';
}

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
