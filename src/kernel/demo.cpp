#include <demo.h>
#include <stdio.h>
#include <fs/ustar/ustar.h>
#include <sys/printk.h>
#include <mm/memory.h>
#include <mm/heap.h>
#include <dev/timer/pit/pit.h>
#include <sys/cstr.h>
#include <math.h>

namespace RAYTRACER {

struct Vec3 {
	double x,y,z;
	Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
	Vec3 operator + (const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
	Vec3 operator - (const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
	Vec3 operator * (double d) const { return Vec3(x*d, y*d, z*d); }
	Vec3 operator / (double d) const { return Vec3(x/d, y/d, z/d); }
	Vec3 normalize() const {
		double mg = sqrt(x*x + y*y + z*z);
		return Vec3(x/mg,y/mg,z/mg);
	}
};
inline double dot(const Vec3& a, const Vec3& b) {
	return (a.x*b.x + a.y*b.y + a.z*b.z);
}

struct Ray {
	Vec3 o,d;
	Ray(const Vec3& o, const Vec3& d) : o(o), d(d) {}
};

struct Sphere {
	Vec3 c;
	double r;
	Sphere(const Vec3& c, double r) : c(c), r(r) {}
	Vec3 getNormal(const Vec3& pi) const { return (pi - c) / r; }
	bool intersect(const Ray& ray, double &t) const {
		const Vec3 o = ray.o;
		const Vec3 d = ray.d;
		const Vec3 oc = o - c;
		const double b = 2 * dot(oc, d);
		const double c = dot(oc, oc) - r*r;
		double disc = b*b - 4 * c;
		if (disc < 1e-4) return false;
		disc = sqrt(disc);
		const double t0 = -b - disc;
		const double t1 = -b + disc;
		t = (t0 < t1) ? t0 : t1;
		return true;
	}
};

void clamp255(Vec3& col) {
	col.x = (col.x > 255) ? 255 : (col.x < 0) ? 0 : col.x;
	col.y = (col.y > 255) ? 255 : (col.y < 0) ? 0 : col.y;
	col.z = (col.z > 255) ? 255 : (col.z < 0) ? 0 : col.z;
}

void Raytrace() {
	const int H = 640;
	const int W = 640;

	const Vec3 white(255, 255, 255);
	const Vec3 black(0, 0, 0);
	const Vec3 red(255, 0, 0);
	const Vec3 green(0, 255, 0);
	const Vec3 blue(0, 0, 255);

	const uint8_t spheresSize = 9;
	const Sphere spheres[spheresSize] = {
		Sphere(Vec3(W*0.25, H*0.25, 60), 50),
		Sphere(Vec3(W*0.25, H*0.75, 60), 50),
		Sphere(Vec3(W*0.25, H*0.50, 60), 50),
		Sphere(Vec3(W*0.50, H*0.50, 60), 50),
		Sphere(Vec3(W*0.50, H*0.25, 60), 50),
		Sphere(Vec3(W*0.50, H*0.75, 60), 50),
		Sphere(Vec3(W*0.75, H*0.50, 60), 50),
		Sphere(Vec3(W*0.75, H*0.25, 60), 50),
		Sphere(Vec3(W*0.75, H*0.75, 60), 50),

	};

	for (double i = 0; i <= 1; i+= 0.25) {
		for (double j = 0; j <= 1; j+=0.25) {
			const Sphere light(Vec3(W * i, H * j, 0), 1);

			double t;
			Vec3 pix_col(black);

			for (int y = 0; y < H; ++y) {
				for (int x = 0; x < W; ++x) {
					pix_col = black;
					const Ray ray(Vec3(x,y,0),Vec3(0,0,1));

					for (int i = 0; i < spheresSize; i++) {
						if (spheres[i].intersect(ray, t)) {
							const Vec3 pi = ray.o + ray.d*t;
							const Vec3 L = light.c - pi;
							const Vec3 N = spheres[i].getNormal(pi);
							const double dt = dot(L.normalize(), N.normalize());
							if (i < 3) pix_col = (red + white*dt) * 0.5;
							else if (i < 6) pix_col = (green + white*dt) * 0.5;
							else pix_col = (blue + white*dt) * 0.5;
							clamp255(pix_col);
							break;
						}
					}

					uint32_t color = ((uint8_t)pix_col.z | ((uint8_t)pix_col.y << 8) | ((uint8_t)pix_col.x << 16) | (0xff << 24));
					GOP_print_pixel(x, y, color);
				}
			}

		}
	}

	return;
}

}

namespace DEMO {

void Raytrace() {
	RAYTRACER::Raytrace();
	return;
}

void Stress() {
        uint32_t malloc_count = 100;
        uint32_t memcpy_count = 100;
        uint32_t memset_count = 100;
        uint32_t memcmp_count = 100;
        uint8_t total_sizes = 14;
        uint32_t sizes[] = {
                512,
                1024,
                2048,
                4096,
                8192,
                16384,
                32768,
                65536,
                131072,
                262144,
                524288,
                1048576,
                2097152,
                4194304
        };

        printf("1. Malloc and free stressing.\n");
        printf("Starting %d mallocs of sizes from 512 to 4M.\n", malloc_count);
        for (int i = 0; i < total_sizes; i++) {
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < malloc_count; j++) {
                        uint8_t *data = (uint8_t*)malloc(sizes[i]);
                        memset(data, 0, sizes[i]);
                        free(data);
                }
                printf("  Done.\n");
        }

        printf("2. Memcpy stressing.\n");
        printf("Starting %d memcpys of sizes from 512 to 4M.\n", memcpy_count);
        for (int i = 0; i < total_sizes; i++) {
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < memcpy_count; j++) {
                        uint8_t *src = (uint8_t*)malloc(sizes[i]);
                        uint8_t *dest = (uint8_t*)malloc(sizes[i]);
                        memcpy(dest, src, sizes[i]);
                        free(src);
                        free(dest);
                }
                printf("  Done.\n");
        }

        printf("2. Memset stressing.\n");
        printf("Starting %d memsets of sizes from 512 to 4M.\n", memset_count);
        for (int i = 0; i < total_sizes; i++) {
                uint8_t *data = (uint8_t*)malloc(sizes[i]);
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < memset_count; j++) {
                        memset(data, 0, sizes[i]);
                }
                printf("  Done.\n");
                free(data);
        }

        printf("3. Memcmp stressing.\n");
        printf("Starting %d memcmps of sizes from 512 to 4M.\n", memcmp_count);
        for (int i = 0; i < total_sizes; i++) {
                uint8_t *src = (uint8_t*)malloc(sizes[i]);
                uint8_t *dest = (uint8_t*)malloc(sizes[i]);
                memset(src, 0, sizes[i]);
                memset(dest, 0, sizes[i]);
                printf(" -> Size: %d ", sizes[i]);
                for (int j = 0; j < memcmp_count; j++) {
                        memcmp(dest, src, sizes[i]);
                }
                printf("  Done.\n");
                free(src);
                free(dest);
        }

}

void Doom() {
}

void BadApple() {
        uint16_t files = 30;
        char *filenames[] = {
                "001.ppm\0",
                "002.ppm\0",
                "003.ppm\0",
                "004.ppm\0",
                "005.ppm\0",
                "006.ppm\0",
                "007.ppm\0",
                "008.ppm\0",
                "009.ppm\0",
                "010.ppm\0",
                "011.ppm\0",
                "012.ppm\0",
                "013.ppm\0",
                "014.ppm\0",
                "015.ppm\0",
                "016.ppm\0",
                "017.ppm\0",
                "018.ppm\0",
                "019.ppm\0",
                "020.ppm\0",
                "021.ppm\0",
                "022.ppm\0",
                "023.ppm\0",
                "024.ppm\0",
                "025.ppm\0",
                "026.ppm\0",
                "027.ppm\0",
                "028.ppm\0",
                "029.ppm\0",
                "030.ppm\0",
        };

        #ifdef KCONSOLE_GOP
                GlobalRenderer.print_clear();
                for (int i = 0; i<30; i++) printf("\n");
        #endif

        for (int i = 0; i<files; i++) {
                size_t size;
                USTAR::GetFileSize(filenames[i], &size);
                uint8_t *buffer = (uint8_t*)malloc(size);
                memset(buffer, 0, size);
                if(USTAR::ReadFile(filenames[i], &buffer, size)) {
                        print_image(buffer);
                }

                free(buffer);
        }
}
}
