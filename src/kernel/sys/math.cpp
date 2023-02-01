#include <math.h>

double sqrt(const double m) {
	double i=0;
	double x1,x2;
	while( (i*i) <= m ) i+=0.1f;

	x1=i;

	for(int j=0;j<10;j++) {
		x2=m;
		x2/=x1;
		x2+=x1;
		x2/=2;
		x1=x2;
	}
	return x2;
}

int sqrt(int x) {
	// Base cases
	if (x == 0 || x == 1)
	return x;

	// Starting from 1, try all numbers until
	// i*i is greater than or equal to x.
	int i = 1, result = 1;
	while (result <= x) {
		i++;
		result = i * i;
	}

	return i - 1;
}
