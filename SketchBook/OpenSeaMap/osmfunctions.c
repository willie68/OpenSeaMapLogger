/**
 * my strcat function.
 **/
void strcat(char* original, char appended)
{
  while (*original++)
    ;
  *original--;
  *original = appended;
  *original++;
  *original = '\0';
}


