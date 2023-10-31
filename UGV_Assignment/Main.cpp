#include <stdio.h>
#include <iostream>

#include "TMM.h"

int main(void) {
	std::cout << "henlo" << std::endl;
	ThreadManagement^ myTMT = gcnew ThreadManagement();

	myTMT->setupSharedMemory();
	myTMT->threadFunction();


	return 0;
}