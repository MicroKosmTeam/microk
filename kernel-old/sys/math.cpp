#include <math.h>

double sqrt(const double m) {
	if (true) { // x86 FPU
		double res;
		asm ("fsqrt" : "=t" (res) : "0" (m));
		return res;
	} else {
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
}
