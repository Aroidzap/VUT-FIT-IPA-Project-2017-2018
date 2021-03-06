/*
*	Soubor: ipa_algorithm_c.c
*	Projekt: Grafický editor: Rotace obrázku s antialiasingem, VUT FIT 2017/2018
*	Autor: Tomáš Pazdiora
*	Login: xpazdi02
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define PI (3.141592653589793)
#define DEG_TO_RAD(x) ((x)*PI/180.0)

__declspec(dllexport) void __cdecl ipa_algorithm_c(unsigned char *, unsigned char *, unsigned int, unsigned int, int, char**);

typedef struct vec2_t {
    float u, v;
} vec2;

typedef struct options_t {
    float angle;
    float scale;
    vec2 pivot;
}options;

typedef float mat2x3[6];

// optimized solution, requires width >= 4
void transform_image_nearest_avx2_fma(unsigned char *input_data, unsigned char *output_data, mat2x3 inv_matrix, unsigned int width, unsigned int height);

// reference solution
void transform_image_nearest(unsigned char *input_data, unsigned char *output_data, mat2x3 inv_matrix, unsigned int width, unsigned int height);

bool load_args(int argc, char **argv, options *args){
    /*
    *   -a -angle ANGLE_DEGREES
    *   -s -scale SCALE
    *   -p -pivot PIVOT_X PIVOT_Y       note: range <0,1>
    */

    // starting from argument #2, ignoring executable path and image input
    for (int i = 2; i < argc; i++) {
        if (strcmp("-a", argv[i]) == 0 || strcmp("-angle", argv[i]) == 0) {
            i++; if (i >= argc) { return false; }
            if (sscanf(argv[i], "%f", &args->angle) != 1) {
                return false;
            }
        }
        else if (strcmp("-s", argv[i]) == 0 || strcmp("-scale", argv[i]) == 0) {
            i++; if (i >= argc) { return false; }
            if (sscanf(argv[i], "%f", &args->scale) != 1) {
                return false;
            }
        }
        else if (strcmp("-p", argv[i]) == 0 || strcmp("-pivot", argv[i]) == 0) {
            i++; if (i >= argc) { return false; }
            if (sscanf(argv[i], "%f", &args->pivot.u) != 1) {
                return false;
            }
            i++; if (i >= argc) { return false; }
            if (sscanf(argv[i], "%f", &args->pivot.v) != 1) {
                return false;
            }
        }
        else {
            return false;
        }
    }
    return true;
}

void _ipa_algorithm_c(unsigned char *input_data, unsigned char *output_data, unsigned int width, unsigned int height, int argc, char** argv)
{
    //defaults
    options arg = { .scale = 1.0f,
                    .angle = 0.0f,
                    .pivot = {.u = 0.5f,.v = 0.5f}  };

	//load arguments
    if (!load_args(argc, argv, &arg)) { 
        fprintf(stderr, "Warning: wrong arguments!\n\n");
        printf("Usage: display_image ImageToLoadAndDisplay [-a|-angle ANGLE_DEGREES] [-s|-scale SCALE] [-p|-pivot PIVOT_X PIVOT_Y]\n\n");
        return;
    }

	//check minimal width (required by transform_image_nearest_avx2_fma)
	if (width < 4) {
		fprintf(stderr, "Error: image must be at least 4 pixels wide!\n\n");
		return;
	}

    float angle_rad = (float)DEG_TO_RAD(arg.angle);
    vec2 scale = { .u = arg.scale,.v = arg.scale };
    vec2 pivot = { .u = arg.pivot.u * width,.v = arg.pivot.v * height };

	// build transformation matrix
	float a = cosf(angle_rad) / scale.u;
	float b = sinf(angle_rad) / scale.v;
	float c = -sinf(angle_rad) / scale.u;
	float d = cosf(angle_rad) / scale.v;
	float u = pivot.u;
	float v = pivot.v;
	mat2x3 transform_matrix_inv = { a, b, u - a*u - b*v, c, d, -c*u + v - d*v };

	// call optimized transformation function
	transform_image_nearest_avx2_fma(input_data, output_data, transform_matrix_inv, width, height);
}

// reference solution
void transform_image_nearest(unsigned char *input_data, unsigned char *output_data, mat2x3 inv_matrix, unsigned int width, unsigned int height) {
	for (unsigned int x = 0; x < width; x++)
	{
		for (unsigned int y = 0; y < height; y++)
		{
			int tx = (int)(inv_matrix[0] * x + inv_matrix[1] * y + inv_matrix[2]);
			int ty = (int)(inv_matrix[3] * x + inv_matrix[4] * y + inv_matrix[5]);

			int id = 3 * (y*width + x);

			if (tx < (int)width && tx >0 && ty < (int)height && ty > 0)
			{
				int tid = 3 * (ty*width + tx);

				output_data[id + 0] = input_data[tid + 0];
				output_data[id + 1] = input_data[tid + 1];
				output_data[id + 2] = input_data[tid + 2];
			}
			else {
				output_data[id + 0] = 0;
				output_data[id + 1] = 0;
				output_data[id + 2] = 0;
			}
		}
	}
}