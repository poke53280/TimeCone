
#pragma once
#include <cmath>
#include <vector>
#include <random>

struct instance_data {
    float data[4];
};

struct arena_vertex {
    float position[3];
    float colorRGBA[4];

    void set_color(float color[4])
    {
        colorRGBA[0] = color[0];
        colorRGBA[1] = color[1];
        colorRGBA[2] = color[2];
        colorRGBA[3] = color[3];
    }

    void transform(float rCos, float rSin, float rDisplace)
    {

        position[0] = position[0] + rDisplace;

        float ap0 = position[0] * rCos - position[1] * rSin;
        float ap1 = position[0] * rSin + position[1] * rCos;

        position[0] = ap0;
        position[1] = ap1;

        position[2] += 0.3f * sqrt(ap0 * ap0 + ap1 * ap1);

    }

};

struct Cube {

    arena_vertex acVertex[8];

    Cube(float x0, float y0, float z0, float x1, float y1, float z1)
    {
        acVertex[0] = { { x0, y0, z0 },{ 0,0,0,0 } };
        acVertex[1] = { { x1, y0, z0 },{ 0,0,0,0 } };
        acVertex[2] = { { x1,  y1, z0 },{ 0,0,0,0 } };
        acVertex[3] = { { x0,  y1, z0 },{ 0,0,0,0 } };

        acVertex[4] = { { x0, y0,  z1 },{ 0,0,0,0 } };
        acVertex[5] = { { x1, y0,  z1 },{ 0,0,0,0 } };
        acVertex[6] = { { x1,  y1,  z1 },{ 0,0,0,0 } };
        acVertex[7] = { { x0,  y1,  z1 },{ 0,0,0,0 } };

    }

    void transform(float rCos, float rSin, float x)
    {
        for (int iVertex = 0; iVertex < 8; iVertex++)
        {
            acVertex[iVertex].transform(rCos, rSin, x);
        }
    }

    void set_color_low_x(float color[4])
    {
        acVertex[0].set_color(color);
        acVertex[3].set_color(color);
        acVertex[4].set_color(color);
        acVertex[7].set_color(color);
    }

    void set_color_hi_x(float color[4])
    {
        acVertex[1].set_color(color);
        acVertex[2].set_color(color);
        acVertex[5].set_color(color);
        acVertex[6].set_color(color);
    }

    void add(std::vector<arena_vertex>& lcVertex)
    {
        for (int iVertex = 0; iVertex < 8; iVertex++)
        {
            lcVertex.push_back(acVertex[iVertex]);
        }
    }

    void set_color(float r, float g, float b)
    {

        float colorNormal[4] = { r, g, b, 0.3f };

        set_color_low_x(colorNormal);
        set_color_hi_x(colorNormal);
    }
};


static void setup_indices(std::vector<uint32_t>& lcIndex, uint32_t num_cubes)
{
    uint32_t iFirstIndex = 0;

    for (uint32_t i = 0; i < num_cubes; i++)
    {
        lcIndex.push_back(iFirstIndex + 0);
        lcIndex.push_back(iFirstIndex + 2);
        lcIndex.push_back(iFirstIndex + 1);

        lcIndex.push_back(iFirstIndex + 0);
        lcIndex.push_back(iFirstIndex + 3);
        lcIndex.push_back(iFirstIndex + 2);

        lcIndex.push_back(iFirstIndex + 1);
        lcIndex.push_back(iFirstIndex + 2);
        lcIndex.push_back(iFirstIndex + 6);

        lcIndex.push_back(iFirstIndex + 6);
        lcIndex.push_back(iFirstIndex + 5);
        lcIndex.push_back(iFirstIndex + 1);

        lcIndex.push_back(iFirstIndex + 4);
        lcIndex.push_back(iFirstIndex + 5);
        lcIndex.push_back(iFirstIndex + 6);

        lcIndex.push_back(iFirstIndex + 6);
        lcIndex.push_back(iFirstIndex + 7);
        lcIndex.push_back(iFirstIndex + 4);

        lcIndex.push_back(iFirstIndex + 2);
        lcIndex.push_back(iFirstIndex + 3);
        lcIndex.push_back(iFirstIndex + 6);

        lcIndex.push_back(iFirstIndex + 6);
        lcIndex.push_back(iFirstIndex + 3);
        lcIndex.push_back(iFirstIndex + 7);

        lcIndex.push_back(iFirstIndex + 0);
        lcIndex.push_back(iFirstIndex + 7);
        lcIndex.push_back(iFirstIndex + 3);

        lcIndex.push_back(iFirstIndex + 0);
        lcIndex.push_back(iFirstIndex + 4);
        lcIndex.push_back(iFirstIndex + 7);

        lcIndex.push_back(iFirstIndex + 0);
        lcIndex.push_back(iFirstIndex + 1);
        lcIndex.push_back(iFirstIndex + 5);

        lcIndex.push_back(iFirstIndex + 0);
        lcIndex.push_back(iFirstIndex + 5);
        lcIndex.push_back(iFirstIndex + 4);

        iFirstIndex += 8;
    }
}

static void create_single_cube(std::vector<arena_vertex>& lcVertex)
{
    const float W2 = 0.5f;
    
    Cube cc(-W2, -W2, -W2, W2, W2, W2);

    cc.transform(0, 1, 0);

    cc.set_color(0.7f, 0.3f, 0.9f);

    cc.add(lcVertex);

}


static void create_cube_arena(std::vector<arena_vertex>& lcVertex)
{
    const float x_0 = -1.0f;
    const float y_0 = -1.f;
    const float z_0 = -1.f;


    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(1, 6);

    std::uniform_real_distribution<float> color_dis(0, 1);
    std::uniform_int_distribution<> distance_dis(1, 10);

    std::uniform_int_distribution<> skip_dis(0, 10);

    // Num rays
    const int NUM_RAY = 5010;

    const float R_FILL_CIRCLE = 0.95f;

    const float rSectorSize = R_FILL_CIRCLE * 1.f / NUM_RAY;

    for (uint32_t iRay = 0; iRay < NUM_RAY; iRay++)
    {

        const float PI = 3.1415927f;

        const float rAngle = rSectorSize * iRay * 2.f * PI;

        const float rCos = cos(rAngle);
        const float rSin = sin(rAngle);

        float x_init = 34.f;
        const float x_offset_step = 9.0f;

        for (uint32_t i = 0; i < 10; i++)
        {
            // Thickness
            const float y_L = 0.08f;
            const float y_1 = y_0 + y_L;

            // Height
            const float z_L = 0.09f;
            const float z_1 = z_0 + z_L;

            const float x_L = 2.0f + dis(gen) * 0.3f;
            const float x_1 = x_0 + x_L;

            const float x_offset_this = x_offset_step + 0.4f * distance_dis(gen);

            float x = x_init + x_offset_this;

            x_init = x;

            if (skip_dis(gen) > 5)
            {
                continue;
            }


            Cube cc(x_0, y_0, z_0, x_1, y_1, z_1);

            cc.transform(rCos, rSin, x);

            cc.set_color(color_dis(gen), color_dis(gen), color_dis(gen));

            cc.add(lcVertex);
        }
    }
}





