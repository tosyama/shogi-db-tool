#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <stdio.h>

FILE *shg_log;

int main(int argc, char* argv[])
{
    shg_log = fopen("test.log", "w");
	int result = Catch::Session().run(argc, argv);
	fclose(shg_log);

	return (result < 0xff ? result : 0xff);
}
