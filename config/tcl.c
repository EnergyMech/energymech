#include <unistd.h>
#include <tcl.h>
Tcl_Interp *interp = NULL;
int main(void)
{ 
interp = Tcl_CreateInterp();
Tcl_Init(interp);
return(0);
} 

