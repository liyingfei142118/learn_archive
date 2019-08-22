#include <stdio.h>
#define LENGTH 100
int main()
{
	FILE *fp = NULL;
	char str[LENGTH];
	fp  = fopen("hello.txt", "a+");
	if(fp)
	{
		fprintf(fp, "fprintf#######\n");
		fputs("fputs########\n", fp);
		fclose(fp);
	}
	fp = fopen("hello.txt", "r");
	int key = fgets(str, LENGTH, fp);
	while(key)
	{	
		printf("%s", str);
		key = fgets(str, LENGTH, fp);
	}
	fclose(fp);
	
	int c;
	fp = fopen("hello.txt", "r");
	c = fgetc(fp);
	while(c != EOF)
	{
		printf("%c", c);
		putchar(c);
		c = fgetc(fp);
	}
	fclose(fp);
}
