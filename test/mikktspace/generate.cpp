#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mikktspace.h"

#include <gtest/gtest.h>
#include "../test_utils.h"

struct tangent {
    // Vector.
    float v[3];

    // Sign.
    float s;
};

struct vertex {
    // Borrows from `input.positions`.
    float (*position)[3];

    // Borrows from `input.positions`.
    float (*normal)[3];

    // Borrows from `input.positions`.
    float (*tex_coord)[2];
};

struct face {
    // Borrows from `positions`, `normals`, and `tex_coords`.
    struct vertex vertices[3];
};

struct input {
    // Owned.
    float (*positions)[3];

    // Owned.
    float (*normals)[3];

    // Owned.
    float (*tex_coords)[2];

    // Borrows from `positions`, `normals`, and `tex_coords`.
    struct face *faces;

    // Number of entries in `positions`, `normals`, and `tex_coords`.
    size_t nr_vertices;

    // Number of entries in `faces`.
    size_t nr_faces;
} input;

void print_vec2(float (*t)[2]) {
    printf("[%f, %f]", (*t)[0], (*t)[1]);
}

void print_vec3(float (*t)[3]) {
    printf("[%f, %f, %f]", (*t)[0], (*t)[1], (*t)[2]);
}

void print_tangent(const struct tangent *t) {
    printf("[%f, %f, %f, %f]", t->v[0], t->v[1], t->v[2], t->s);
}

int get_num_faces(const SMikkTSpaceContext *x) {
    return input.nr_faces;
}

int get_num_vertices_of_face(const SMikkTSpaceContext *x, int f) {
    return 3;
}

void get_position(const SMikkTSpaceContext *x, float *dst, int f, int v) {
    float (*src)[3] = input.faces[f].vertices[v].position;
    memcpy(dst, *src, 3 * sizeof(float));
}

void get_normal(const SMikkTSpaceContext *x, float *dst, int f, int v) {
    float (*src)[3] = input.faces[f].vertices[v].normal;
    memcpy(dst, *src, 3 * sizeof(float));
}

void get_tex_coord(const SMikkTSpaceContext *x, float *dst, int f, int v) {
    float (*src)[2] = input.faces[f].vertices[v].tex_coord;
    memcpy(dst, *src, 2 * sizeof(float));
}

void set_tspace_basic(
    const SMikkTSpaceContext *x,
    const float *t, 
    float s, 
    int f, 
    int v
) {
    // The index of the last output (vertex, tangent) pair.
    static int i = 0;

    struct vertex *in = &input.faces[f].vertices[v];

    printf("%d: { v: ", i);
    print_vec3(in->position);
    printf(", vn: ");
    print_vec3(in->normal);
    printf(", vt: ");
    print_vec2(in->tex_coord);
    printf(", vx: [%f, %f, %f, %f] }", t[0], t[1], t[2], s);
    puts("");

    ++i;
}

void set_tspace(
    const SMikkTSpaceContext *x, 
    const float *t,
    const float *b, 
    float mag_s, 
    float mag_t,
    tbool op,
    int f, 
    int v
) {
    set_tspace_basic(x, t, op != 0 ? 1.0f : -1.0f, f, v);
}



TEST(MIKKTSPACE, cubeTest) {
    input.nr_vertices = 30;
    input.nr_faces = 24;

    input.positions = (float(*)[3])calloc(input.nr_vertices, sizeof(*input.positions));
    input.normals = (float(*)[3])calloc(input.nr_vertices, sizeof(*input.normals));
    input.tex_coords = (float(*)[2])calloc(input.nr_vertices, sizeof(*input.tex_coords));
    input.faces = (face*)calloc(input.nr_faces, sizeof(*input.faces));

	path in_path = test_resources_path / "objs" / "cube.obj";

    FILE *fi = fopen(in_path.string().c_str(), "rb");
    assert(fi);
    char buffer[1024];

    for (size_t i = 0; i < input.nr_vertices; ++i) {
        fgets(buffer, sizeof(buffer), fi);
        sscanf(
            buffer, 
            "v %f %f %f",
            &input.positions[i][0],
            &input.positions[i][1],
            &input.positions[i][2]
        );
    }

    for (size_t i = 0; i < input.nr_vertices; ++i) {
        fgets(buffer, sizeof(buffer), fi);
        sscanf(
            buffer, 
            "vn %f %f %f",
            &input.normals[i][0], 
            &input.normals[i][1],
            &input.normals[i][2]
        );
    }
    
    for (size_t i = 0; i < input.nr_vertices; ++i) {
        fgets(buffer, sizeof(buffer), fi);
        sscanf(
            buffer, 
            "vt %f %f",
            &input.tex_coords[i][0], 
            &input.tex_coords[i][1]
        );
    }

    for (size_t i = 0; i < input.nr_faces; ++i) {
        fgets(buffer, sizeof(buffer), fi);
        int v[3];
        sscanf(
            buffer, 
            "f %d/%d/%d %d/%d/%d %d/%d/%d",
            &v[0], &v[0], &v[0],
            &v[1], &v[1], &v[1],
            &v[2], &v[2], &v[2]
        );
        for (size_t j = 0; j < 3; ++j) {
            input.faces[i].vertices[j].position = &input.positions[v[j] - 1];
            input.faces[i].vertices[j].normal = &input.normals[v[j] - 1];
            input.faces[i].vertices[j].tex_coord = &input.tex_coords[v[j] - 1];
        }
    }

	SMikkTSpaceInterface interface;

	interface.m_getNumFaces = get_num_faces;
	interface.m_getNumVerticesOfFace = get_num_vertices_of_face;
	interface.m_getPosition = get_position;
	interface.m_getNormal = get_normal;
	interface.m_getTexCoord = get_tex_coord;
	interface.m_setTSpaceBasic = set_tspace_basic;
	interface.m_setTSpace = set_tspace;

    //SMikkTSpaceInterface interface = {
    //    .m_getNumFaces = get_num_faces,
    //    .m_getNumVerticesOfFace = get_num_vertices_of_face,
    //    .m_getPosition = get_position,
    //    .m_getNormal = get_normal,
    //    .m_getTexCoord = get_tex_coord,
    //    .m_setTSpaceBasic = set_tspace_basic,
    //    .m_setTSpace = set_tspace,
    //};

	SMikkTSpaceContext context;
	context.m_pInterface = &interface;
	context.m_pUserData = NULL;

    //SMikkTSpaceContext context = {
    //    .m_pInterface = &interface,
    //    .m_pUserData = NULL,
    //};

    genTangSpaceDefault(&context);

    fclose(fi);
    free(input.positions);
    free(input.normals);
    free(input.tex_coords);
    free(input.faces);

//    return 0;
}

#include <igl/gaussian_curvature.h>
#include <igl/massmatrix.h>
#include <igl/invert_diag.h>
#include <igl/readOFF.h>
#include <igl/jet.h>

static const path test_libigl_tutorial_path = "..\\..\\..\\lib\\libigl\\tutorial\\shared\\";

TEST(LIBIGL, gaussianCurvature) 
{
	using namespace Eigen;
	using namespace std;
	MatrixXf V;
	MatrixXi F;
	path mesh = test_libigl_tutorial_path / "bumpy.off";
	ASSERT_TRUE(exists(mesh));
	igl::readOFF(mesh.string(), V, F);

	VectorXf K;
	// Compute integral of Gaussian curvature
	igl::gaussian_curvature(V, F, K);
	// Compute mass matrix
	SparseMatrix<float> M, Minv;
	igl::massmatrix(V, F, igl::MASSMATRIX_TYPE_DEFAULT, M);
	igl::invert_diag(M, Minv);
	// Divide by area to get integral average
	K = (Minv*K).eval();

}
