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

/** ********************************
 * Demonstration of Simplest Usage *
 **********************************/

/** @brief Callback function for ri_read_file(), called from main() **/
void use_sections(const ri_Section* sections, void *data)
{
   const ri_Section *sptr = sections;

   printf("Showing contents of configuration file, comments removed.\n");

   while (sptr)
   {
      show_section(sptr->section_name, sptr->lines);
      sptr = sptr->next;
   }

   printf("\n\n[32;1mWithin full-read pattern, testing value acquisition.\n[m");

   printf("Using ri_find_section_value to get something\n");
   printf("Getting bogus/user : '%s'.\n", ri_find_section_value(sections, "bogus", "user"));
   printf("Getting bogus/from : '%s'.\n", ri_find_section_value(sections, "bogus", "from"));
   printf("Getting bogus/password : '%s'.\n", ri_find_section_value(sections, "bogus", "password"));

   printf("\n\n[32;1mTesting ri_get_section() on \"fake\" section.[m\n");
   sptr = ri_get_section(sections, "fake");
   if (sptr)
   {
      printf("Found \"fake\" section.  About to iterate through its contents.\n");
      const ri_Line *line = sptr->lines;
      while (line)
      {
         printf("tag \"%s\" is \"%s\"\n", line->tag, line->value);
         line = line->next;
      }
   }
}

/** **********************************
 * Demonstration of By-Section Usage *
 ************************************/

// Structure passed in void* parameter for use_file_to_read(), use_global() and use_user()
struct payload
{
   const ri_Line *global;
   const char *user;
};


void use_user_account(int fd, const ri_Line* lines, void* data)
{
   // Save the lines to the data structure to be used by the next callback:
   struct payload* pl = (struct payload*)data;

   // (Optional) Make lists aliases to identify contents
   const ri_Line* global = pl->global; 
   const ri_Line* user = lines;

   // We don't have to test this because we wouldn't
   // be here if global didn't include *default-account*.
   const char* account_name = ri_find_value(global, "default-account");

   // Now you can access both the content from [global] and the
   // user account indicated in the [global] section

   printf("Section [global] contents.\n");
   show_section("global", global);

   printf("Default account contents.\n");
   show_section(account_name, user);
}

void use_global_section(int fd, const ri_Line* lines, void* data)
{
   // Save the lines to the data structure to be used by the next callback:
   struct payload* pl = (struct payload*)data;
   pl->global = lines;

   const char *account_name = ri_find_value(pl->global, "default-account");
   if (account_name)
   {
      // Pass the parameter, which is the expected type:
      ri_open_section(fd, account_name, use_user_account, data);
   }
}

/**
 * @brief Demonstration of how ri_open_section may be used
 *        for multiple sections with one file descriptor.
 */
void use_file_to_read(int fd, void* data)
{
   struct payload pl = { NULL, NULL };

   // We don't need to interpret the data here, just pass it on
   ri_open_section(fd, "global", use_global_section, (void*)&pl);
}





int main(int argc, char** argv)
{
   printf("[32;1mTesting recommended full-read configuration file pattern.\n[m");
   ri_read_file("./demo.ini", use_sections, NULL);

   // Test service function
   printf("\n\n[32;1mTesting alternate sparse section reading pattern.\n[m");
   ri_open("./demo.ini", use_file_to_read, NULL);
   
   return 0;
}

