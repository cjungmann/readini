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
   
   return 0;
}
