#include <stdio.h>
#include <stdlib.h>
int main(int argc, char const *argv[])
{
    char* text = NULL;
    text = (char *)malloc(sizeof(char));
    sprintf(text, "Worker successfully created with the pid 5.");
    printf("%s", text);

    return 0;
}
