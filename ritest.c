// -*- compile-command: "cc -Wall -Werror -o ritest ritest.c -lreadini" -*-

#include <stdio.h>
#include <readini.h>

void use_section(int fd, const  char *section_name, LineUser lu)
{
   printf("in ritest, having found a section.\n");
}

void use_file_simple(int fd)
{
   printf("in ritest, having gotten a file.\n");
   seek_section(fd, "global", use_section, NULL);
}

void use_iniline(const struct iniline *lines)
{
   const struct iniline *ptr = lines;

   while (ptr)
   {
      printf("[33;1;44m%s[m", ptr->tag);
      if (ptr->value)
         printf(" : [34;1;44m%s[m", ptr->value);
      printf("\n");

      ptr = ptr->next;
   }
}

void use_inifile(const struct inisection *section)
{
   const struct inisection *s_ptr = section;
   const struct iniline *i_ptr;
   while (s_ptr)
   {
      printf("[34;1m%s[m\n", s_ptr->section_name);

      i_ptr = s_ptr->lines;
      while (i_ptr)
      {
         printf("  %s", i_ptr->tag);
         if (i_ptr->value)
            printf(" : %s", i_ptr->value);
         printf("\n");
         
         i_ptr = i_ptr->next;
      }

      s_ptr = s_ptr->next;
   }
}


void use_file_to_read(int fd)
{
   read_section(fd, "bogus", use_iniline);
}

int main(int argc, char** argv)
{
   printf("In ritest main().\n");

   /* // Test fundamental functions */
   /* cb_open("./demo.ini", use_file_simple); */

   // Test service function
   cb_open("./demo.ini", use_file_to_read);

   printf("Finished with first test.  Now on to test that reads the entire file.\n");

   get_inifile("./demo.ini", use_inifile);

   
   
   return 0;
}
