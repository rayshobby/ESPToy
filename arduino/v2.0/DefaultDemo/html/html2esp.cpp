#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void file_error(const char* name) {
  printf("Can't open file %s\n", name);
}

int main(int argc, char* argv[])
{
  if(argc<3) {
    printf("Usage: %s in.html out\n", argv[0]);
    return 0;
  }
  FILE *fp = fopen(argv[1], "rb");
  if(!fp) {
    file_error(argv[1]);
    return 0;
  }
  char oname[100];
  strcpy(oname, "../");
  strcat(oname, argv[2]);
  strcat(oname, ".h");
  FILE *op = fopen(oname, "wb");
  if(!op) {
    file_error(oname);
    return 0;
  }
  char in[1000];
  char out[1000];
  int size;
  char c;
  int i;
  fprintf(op, "const char %s[] PROGMEM = R\"(", argv[2]);
  while(!feof(fp)) {
  	fgets(in, sizeof(in), fp);
  	size = strlen(in);
  	c = in[size-1];
  	if(c=='\r' || c=='\n') size--;
  	c = in[size-1];
  	if(c=='\r' || c=='\n') size--;
  	char *outp = out;
    bool isEmpty = true;
  	for(i=0;i<size;i++) {
  	  switch(in[i]) {
  	  case ' ':
  	  case '\t':
  	  case '\r':
  	  case '\n':  // remove starting empty chars
  	  case '\0':
  	    if(!isEmpty)
  	      *outp++ = in[i];
  	    break;
  		default:
  		  *outp++ = in[i];
  		  isEmpty = false;
  		  break;
  	  } 
  	}
  	if(size) {
  	  if(strncmp(in, "</body>", 7)==0) {
  	    *outp++ = ')';
  	    *outp++ = '\"';
  	    *outp++ = ';';
  	  }
    	*outp++ = '\r';
    	*outp++ = '\n';
    	*outp++ = 0;
    	fprintf(op, "%s", out);
    }
  }
  fclose(fp);
  fclose(op);
}
