// C++ include
#include <iostream>
#include <string>
#include <vector>

// Utilities for the Assignment
#include "raster.h"

#include <gif.h>
#include <fstream>

#include <Eigen/Geometry>
// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"

using namespace std;
using namespace Eigen;

// Image height
const int H = 480;

// Camera settings
const float near_plane = 1.5; // AKA focal length
const float far_plane = near_plane * 100;
const float field_of_view = 0.7854; // 45 degrees
const float aspect_ratio = 1.5;
const bool is_perspective = false;
const Vector3f camera_position(0, 0, 3);
const Vector3f camera_gaze(0, 0, -1);
const Vector3f camera_top(0, 1, 0);

// Object
const std::string data_dir = DATA_DIR;
const std::string mesh_filename(data_dir + "bunny.off");
MatrixXd vertices; // n x 3 matrix (n points)
MatrixXi facets;   // m x 3 matrix (m triangles)

// Material for the object
const Vector3d obj_diffuse_color(0.5, 0.5, 0.5);
const Vector3d obj_specular_color(0.2, 0.2, 0.2);
const double obj_specular_exponent = 256.0;

// Lights
std::vector<Vector3d> light_positions;
std::vector<Vector3d> light_colors;
// Ambient light
const Vector3d ambient_light(0.3, 0.3, 0.3);

// Fills the different arrays
void setup_scene()
{
    // Loads file
    std::ifstream in(mesh_filename);
    if (!in.good())
    {
        std::cerr << "Invalid file " << mesh_filename << std::endl;
        exit(1);
    }
    std::string token;
    in >> token;
    int nv, nf, ne;
    in >> nv >> nf >> ne;
    vertices.resize(nv, 3);
    facets.resize(nf, 3);
    for (int i = 0; i < nv; ++i)
    {
        in >> vertices(i, 0) >> vertices(i, 1) >> vertices(i, 2);
    }
    for (int i = 0; i < nf; ++i)
    {
        int s;
        in >> s >> facets(i, 0) >> facets(i, 1) >> facets(i, 2);
        assert(s == 3);
    }

    // Lights
    light_positions.emplace_back(8, 8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(6, -8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(4, 8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(2, -8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(0, 8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(-2, -8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(-4, 8, 0);
    light_colors.emplace_back(16, 16, 16);
}

void build_uniform(UniformAttributes &uniform)
{
    // TODO: setup uniform
    uniform.view_transform << 1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

    // TODO: setup camera, compute w, u, v
    const Vector3f w = -1.0 * camera_gaze / camera_gaze.norm();
    const Vector3f u = camera_top.cross(w) / camera_top.cross(w).norm();
    const Vector3f v = w.cross(u);

    // TODO: compute the camera transformation
    Matrix4f temp;
    temp << u(0), v(0), w(0), camera_position(0),
        u(1), v(1), w(1), camera_position(1),
        u(2), v(2), w(2), camera_position(2),
        0, 0, 0, 1;

    const Matrix4f C = temp.inverse();

    // TODO: setup projection matrix
    const double n = -near_plane;
    const double f = -far_plane;
    const double t = near_plane * tan(field_of_view / 2);
    const double r = t * aspect_ratio;
    const double b = -t;
    const double l = -r;

    Matrix4f P;
    if (is_perspective)
    {
        // TODO setup prespective camera
    }
    else
    {
        Matrix4f ortho;
        P << (2 / (r - l)), 0, 0, -((r + l) / (r - l)),
            0, (2 / (t - b)), 0, -((t + b) / (t - b)),
            0, 0, (2 / (n - f)), -((n + f) / (n - f)),
            0, 0, 0, 1;
    }

    uniform.view_transform = P * C;
    std::cout << uniform.view_transform << std::endl;
}

void simple_render(Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;

    program.VertexShader = [](const VertexAttributes &va, const UniformAttributes &uniform)
    {
        // TODO: fill the shader
        VertexAttributes out;
        out.position = uniform.view_transform * va.position;
        return out;
    };

    program.FragmentShader = [](const VertexAttributes &va, const UniformAttributes &uniform)
    {
        // TODO: fill the shader
        return FragmentAttributes(1, 0, 0);
    };

    program.BlendingShader = [](const FragmentAttributes &fa, const FrameBufferAttributes &previous)
    {
        // TODO: fill the shader
        return FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
    };

    std::vector<VertexAttributes> vertex_attributes;
    // TODO: build the vertex attributes from vertices and facets
    for (int i = 0; i < facets.rows(); i++)
    {
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2)));
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2)));
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2)));
    }

    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

Matrix4d compute_rotation(const double alpha)
{
    // TODO: Compute the rotation matrix of angle alpha on the y axis around the object barycenter
    Matrix4d res;

    return res;
}

void wireframe_render(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;

    Matrix4d trafo = compute_rotation(alpha);

    program.VertexShader = [](const VertexAttributes &va, const UniformAttributes &uniform)
    {
        // TODO: fill the shader
        VertexAttributes out;
        out.position = uniform.view_transform * va.position;
        return out;
    };

    program.FragmentShader = [](const VertexAttributes &va, const UniformAttributes &uniform)
    {
        // TODO: fill the shader
        return FragmentAttributes(1, 0, 0);
    };

    program.BlendingShader = [](const FragmentAttributes &fa, const FrameBufferAttributes &previous)
    {
        // TODO: fill the shader
        return FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
    };

    std::vector<VertexAttributes> vertex_attributes;

    // TODO: generate the vertex attributes for the edges and rasterize the lines
    // TODO: use the transformation matrix
    for (int i = 0; i < facets.rows(); i++)
    {
        // Edge 1
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2)));
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2)));

        // Edge 2
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2)));
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2)));

        // Edge 3
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2)));
        vertex_attributes.emplace_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2)));
    }

    rasterize_lines(program, uniform, vertex_attributes, 0.5, frameBuffer);
}

void get_shading_program(Program &program)
{
    program.VertexShader = [](const VertexAttributes &va, const UniformAttributes &uniform)
    {
        // TODO: transform the position and the normal
        // TODO: compute the correct lighting
        return va;
    };

    program.FragmentShader = [](const VertexAttributes &va, const UniformAttributes &uniform)
    {
        // TODO: create the correct fragment
        return FragmentAttributes(1, 0, 0);
    };

    program.BlendingShader = [](const FragmentAttributes &fa, const FrameBufferAttributes &previous)
    {
        // TODO: implement the depth check
        return FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
    };
}

void flat_shading(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;
    get_shading_program(program);
    Eigen::Matrix4d trafo = compute_rotation(alpha);

    std::vector<VertexAttributes> vertex_attributes;
    // TODO: compute the normals
    // TODO: set material colors

    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

void pv_shading(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;
    get_shading_program(program);

    Eigen::Matrix4d trafo = compute_rotation(alpha);

    // TODO: compute the vertex normals as vertex normal average

    std::vector<VertexAttributes> vertex_attributes;
    // TODO: create vertex attributes
    // TODO: set material colors

    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

int main(int argc, char *argv[])
{
    setup_scene();

    int W = H * aspect_ratio;
    Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> frameBuffer(W, H);
    vector<uint8_t> image;

    simple_render(frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("simple.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);

    frameBuffer.setConstant(FrameBufferAttributes());

    wireframe_render(0, frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("wireframe.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);

    // flat_shading(0, frameBuffer);
    // framebuffer_to_uint8(frameBuffer, image);
    // stbi_write_png("flat_shading.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);

    // pv_shading(0, frameBuffer);
    // framebuffer_to_uint8(frameBuffer, image);
    // stbi_write_png("pv_shading.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);

    // TODO: add the animation

    return 0;
}
