#ifndef CSGTESTCORE_H_
#define CSGTESTCORE_H_

enum test_type_e {
	TEST_THROWNTOGETHER,
	TEST_OPENCSG
};

int csgtestcore(int argc, char *argv[], test_type_e test_type);

#endif

