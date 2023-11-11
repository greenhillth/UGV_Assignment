#include <stdio.h>
#include <iostream>

#include "TMM.h"

int main(void) {
	ThreadManagement^ UGV = gcnew ThreadManagement();

	UGV->run();


	return 0;
}