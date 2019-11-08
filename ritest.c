// -*- compile-command: "cc -Wall -Werror -ggdb -o ritest ritest.c -lreadini" -*-

#include <stdio.h>
#include <string.h>  // for strlen()
#include <readini.h>

/**
 * Called by use_global_section and use_bogus_section for a
 * formalized method of displaying the section contents.
 */
void show_section(const char *section_name, const ri_Line* lines)
{
   const ri_Line* ptr = lines;
   int cur_tag_length, max_tag_length = 0;
   char tag_format_buffer[20];

   printf("[34;1m%s[m\n", section_name);
   
   while (ptr)
   {
      cur_tag_length = strlen(ptr->tag);
      if (cur_tag_length > max_tag_length)
         max_tag_length = cur_tag_length;

      ptr = ptr->next;
   }

   sprintf(tag_format_buffer, "  %%%ds", max_tag_length);

   ptr = lines;
   while (ptr)
   {
      printf(tag_format_buffer, ptr->tag);
      if (ptr->value)
         printf(" : %s", ptr->value);
      printf("\n");
         
      ptr = ptr->next;
   }

   printf("\n");
}

void use_global_section(const ri_Line* lines) { show_section("global", lines); }
void use_bogus_section(const ri_Line* lines)  { show_section("bogus", lines); }

/**
 * @brief Demonstration of how ri_open_section may be used
 *        for multiple sections with one file descriptor.
 */
void use_file_to_read(int fd)
{
  ri_open_section(fd, "bogus", use_bogus_section);
  ri_open_section(fd, "global", use_global_section);
}

void use_sections(const ri_Section* sections)
{
   const ri_Section *sptr = sections;

   printf("Showing contents of configuration file, comments removed.\n");

   while (sptr)
   {
      show_section(sptr->section_name, sptr->lines);
      sptr = sptr->next;
   }

   printf("Using ri_find_section_value to get something\n");
   printf("Getting bogus/user : '%s'.\n", ri_find_section_value(sections, "bogus", "user"));
   printf("Getting bogus/from : '%s'.\n", ri_find_section_value(sections, "bogus", "from"));
   printf("Getting bogus/password : '%s'.\n", ri_find_section_value(sections, "bogus", "password"));
}

int main(int argc, char** argv)
{
   // Test service function
   printf("Testing multi-use file descriptor to view sections.\n");
   ri_open("./demo.ini", use_file_to_read);

   printf("Finished with first test.  Now on to test that reads the entire file.\n");
   ri_open_file("./demo.ini", use_sections);

   
   
   return 0;
}
